#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "download.h"
#include "peerlist.h"
#include "peer_server.h"

extern int sock;
extern PeerList peerlist;
extern ChunkList getchunks;

int find_unfetched_chunk(int p_index) {
    int i;
    download_t *dl;
    ll_Node *iter;
    ChunkLine *cl;

    if(p_index < 0) {
        return -1;
    }

    dl = & peerlist.peers[p_index].dl;
    
    iter = ll_start(&dl->queue);
    while(iter != ll_end(&dl->queue)) {
        cl = (ChunkLine *) iter->item;
        for (i = 0; i < getchunks.count; i++) {
            if(getchunks.chunks[i].state == unfetched
                    && strcmp(getchunks.chunks[i].sha1, cl->sha1) == 0) {
                ll_remove(&dl->queue, iter);
                dl->get_index = i;
                return i;
            }
        }
        iter = ll_next(iter);
    }

    return -1;
}

/**
 * init the receiver handler
 * @param dl, the handler
 * @param p_index, the peer index
 * @param get_index, the chunk index
 * @param filename, the target filename
 * @return 0 on success, -1 if fails
 */
int dl_init(download_t *dl, int p_index, int get_index, const char *filename) {
    memset(dl, 0, sizeof(download_t));
    if (strlen(filename) >= BT_FILENAME_LEN) {
        logger(LOG_ERROR, "filename too long");
        return -1;
    }
    
    init_linkedlist(& dl->queue);
    
    dl->next_pkt_expected = 1;
    
    dl->p_index = p_index;
    dl->get_index = get_index;
    dl->ts = get_timestamp_now();
    strcpy(dl->filename, filename);
    
    //BM_CLR(dl->bm); // actually do nothing
    return 0;
}

/**
 * get the current ack that I should send bak
 * @param dl, the handler
 * @return the ack
 */
static uint32_t get_ack(download_t *dl) {
    int i;
    
    for (i = GET_BITMAP_OFFSET(dl, dl->next_pkt_expected) ; i < BT_CHUNK_SIZE; i++) {
        if (!BM_ISSET(dl->bm, i)) {
            break;
        }
    }

    // update next_pkt_expected
    if (dl->started) {
        dl->next_pkt_expected = REVERSE_BITMAP_OFFSET(dl, i);
    }

    return dl->next_pkt_expected - 1;
}

/**
 * send an ack packet to peer[p_index]
 * @param p_index, the index of the pper
 * @param ack, the ack number
 * @return 0 on success, -1 if fails
 */
static int send_ack(int p_index, int ack) {
    pkt_param_t param;

    PKT_PARAM_CLEAR(&param);

    param.socket = sock;
    //param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = 1;
    param.ack = ack;
    param.type = PACKET_TYPE_ACK;

    send_packet(&param);

    logger(LOG_DEBUG, "send ack: %d", param.ack);

    return 0;
}

/**
 * receive the packet, save it into buffer
 * send corresponding ack back
 * @param dl, the handler
 * @param pkt, the received packet
 * @return 0 on success, -1 if fails
 */
int dl_recv(download_t *dl, packet_t *pkt) {
    int i;
    uint32_t offset;

    if (dl->finished) {
        return 0;
    }
    
    /* do not update rtt until start */
    if (dl->started && !dl->block_update) {
        update_rtt(&dl->rtt, &dl->dev, dl->ts);
    } else { /* record data_len for the first time */
        dl->data_length = GET_DATA_LEN(pkt);
        dl->started = 1;
        dl->block_update = 0;
    }
    
    offset = GET_BITMAP_OFFSET(dl, GET_SEQ(pkt));
    printf("offset: %d, len: %d\n", offset, GET_DATA_LEN(pkt));
    if (GET_SEQ(pkt) <= 0 || offset + GET_DATA_LEN(pkt) > BT_CHUNK_SIZE) {
        logger(LOG_ERROR, "data offset out of buffer");
        return -1;
    }
    
    memcpy(dl->buffer + offset, GET_DATA(pkt), GET_DATA_LEN(pkt));

    /* set bitmap */
    for (i = 0; i < GET_DATA_LEN(pkt); i++) {
        BM_SET(dl->bm, offset + i);
    }
    
    send_ack(dl->p_index, get_ack(dl));

    dl->ts = get_timestamp_now();
    dl->timeout_cnt = 0;

    if (GET_BITMAP_OFFSET(dl, dl->next_pkt_expected) >= BT_CHUNK_SIZE) {
        dl->finished = 1;
    }

    return 0;
}

/**
 * handle packet loss case (timeout)
 * @param dl
 */
static void dl_loss(download_t *dl) {
    int i ;

    for (i = 0; i < LOSS_ACK_NUM; i++) {
        send_ack(dl->p_index, get_ack(dl));
    }
    dl->ts = get_timestamp_now();
    dl->block_update = 1;
}

/**
 * check if the connection is timeout
 * @param dl, the handler
 * @return the number of the continuous timouts, -1 if already finished
 */
int dl_check_timeout(download_t *dl) {
    uint32_t now;

    /* test if finished */
    if (dl->finished) {
        return -1;
    }
    
    now = get_timestamp_now();

    /* bootstrap for rtt */
    if ((0 == dl->rtt) && (now - dl->ts) < DEFAULT_TIMEOUT) {
        return dl->timeout_cnt;
    }
    
    //if ((now - dl->ts) < GET_RTO(dl)) {
    if ((now - dl->ts) < DEFAULT_TIMEOUT/2) {
        return dl->timeout_cnt;
    }

    dl->timeout_cnt++;
    dl_loss(dl);
    
    return dl->timeout_cnt;
}

/**
 * write the buffer to file
 * @return 0 on success -1 if fails
 */
int dl_save_buffer(download_t *dl) {
    int fd;
    ssize_t ret;
    ssize_t remain = BT_CHUNK_SIZE;
    
    fd = creat(dl->filename, 0666);
    if (fd < 0) {
        logger(LOG_ERROR, "open() error");
        return -1;
    }

WRITE_LOOP:
    while (remain > 0) {
        ret = write(fd, dl->buffer + (BT_CHUNK_SIZE - remain), BT_CHUNK_SIZE);
        if (ret < 0) {
            if (EINTR == ret) {
                remain = BT_CHUNK_SIZE;
                goto WRITE_LOOP;
            } else {
                logger(LOG_ERROR, "write() error");
                perror("write");
                return -1;
            }
        }
        
        remain -= ret;
    }

    if (close(fd) < 0) {
        logger(LOG_ERROR, "close() error");
        return -1;
    }
    
    return 0;
}

/**
 * check the hach
 * @return 0 on success, -1 if the hash not equal
 */
int check_hash() {
    return 0;
}

/**
 * a wrapper for finishing the downloading
 * @param dl, the handler
 * @return 0 on success, 1 if check_hash fails, -1 if other IO fails
 */
int dl_finish(download_t *dl) {
    if (0 != check_hash()) {
        return 1;
    }

    return dl_save_buffer(dl);
}

/**
 * a debugging helper
 * @param dl, the handler
 */
void dl_dump(download_t *dl, FILE *fp) {
    fprintf(fp, "-----------------\n");
    fprintf(fp, "| p_index: %d\t|\n", dl->p_index);
    fprintf(fp, "| get_index: %d\n", dl->get_index);
    fprintf(fp, "| ts: %u\n", dl->ts);
    fprintf(fp, "| rtt: %u\t|\n", dl->rtt);
    fprintf(fp, "| dev: %u\t|\n", dl->dev);
    fprintf(fp, "| timeout_cnt: %d\t|\n", dl->timeout_cnt);
    fprintf(fp, "| filename: %s\t|\n", dl->filename);
    fprintf(fp, "| started: %d\n", dl->started);
    fprintf(fp, "| finished: %d\n", dl->finished);
    fprintf(fp, "| next_pkt_expected : %d\n", dl->next_pkt_expected);
    fprintf(fp, "| data_length: %lu\n", dl->data_length);
}
