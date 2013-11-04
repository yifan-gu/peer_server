#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "download.h"
#include "peerlist.h"
#include "peer_server.h"

extern PeerServer psvr;



/**
 * init the receiver handler
 * @param dl, the handler
 * @param p_index, the peer index
 * @param get_index, the chunk index
 * @param filename, the target filename
 * @return 0 on success, -1 if fails
 */
int dl_init(download_t *dl, int p_index, int get_index, const char filename[BT_CHUNK_SIZE]) {
    memset(dl, 0, sizeof(download_t));

    dl->next_pkt_expected = 1;

    dl->p_index = p_index;
    dl->get_index = get_index;
    dl->ts = get_timestamp_now();

    strcpy(dl->filename, filename);

    init_recvwin(&dl->rwin);
    return 0;
}

/**
 * get the current ack that I should send bak
 * @param dl, the handler
 * @return the ack
 */
static uint32_t get_ack(download_t *dl) {
    recvwin_slideack(&dl->rwin);
    dl->next_pkt_expected = dl->rwin.next_seq;

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

    param.socket = psvr.sock;
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
    uint32_t seq;
    size_t offset;

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

    seq = GET_SEQ(pkt);
    offset = (seq - 1) * dl->data_length ;
    // discard data packet with seq number not sitting in current receive window
    //   or size exceeds the buffer length.
    if( ! seq_fit_in(&dl->rwin, seq) || offset + GET_DATA_LEN(pkt) > BT_CHUNK_SIZE){
        return -1;
    }

    if(! seq_exist_in(&dl->rwin, seq)){
        memcpy(dl->buffer + offset, GET_DATA(pkt), GET_DATA_LEN(pkt));
        recvwin_mark(&dl->rwin, seq);
    }

    send_ack(dl->p_index, get_ack(dl));

    dl->ts = get_timestamp_now();
    dl->timeout_cnt = 0;

    if ( (dl->next_pkt_expected -1)* dl->data_length >= BT_CHUNK_SIZE) {
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

    fd = open(dl->filename,
              O_WRONLY | O_CREAT,
              0644);
    if (fd < 0) {
        logger(LOG_ERROR, "open file (%s) error", dl->filename);
        return -1;
    }

    if(lseek(fd,
             psvr.getchunks.chunks[dl->get_index].id * BT_CHUNK_SIZE,
             SEEK_SET) == (off_t) -1) {
        logger(LOG_ERROR, "seek file (%s) error: offset at %d",
               dl->filename,
               psvr.getchunks.chunks[dl->get_index].id * BT_CHUNK_SIZE);
    }

    if( write(fd, dl->buffer, BT_CHUNK_SIZE) == BT_CHUNK_SIZE ) {
        // mark in getchunks as fetched
        psvr.getchunks.chunks[dl->get_index].state = fetched;
        psvr.dl_remain --;
        if(psvr.dl_remain == 0){
            printf("GOT %s\n", psvr.getchunk_file);
        }
        // insert to local haschunks
        add_chunk(&psvr.haschunks, &psvr.getchunks.chunks[dl->get_index]);
    }else{
        logger(LOG_ERROR, "writing to file (%s) error for chunk (%d)",
               dl->filename, psvr.getchunks.chunks[dl->get_index].id);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

/**
 * check the hash
 * @return 0 on success, -1 if the hash not equal
 */
int dl_check_hash(void) {
    return 0;
}

/**
 * a wrapper for finishing the downloading
 * @param dl, the handler
 * @return 1 on success, 0 on failure, -1 on error;
 */
/*int dl_finish(download_t *dl) {*/
/*return dl_save_buffer(dl);*/
/*}*/

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
