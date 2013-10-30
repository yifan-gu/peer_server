#include "tcp_recv.h"
#include "tcp_util.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

extern int sock;
extern PeerList peerlist;

/**
 * init the receiver handler
 * @param tcp, the handler
 * @param p_index, the peer index
 * @param c_index, the chunk index
 * @param filename, the target filename
 * @return 0 on success, -1 if fails
 */
int init_tcp_recv(tcp_recv_t *tcp, int p_index, int c_index, const char *filename) {
    memset(tcp, 0, sizeof(tcp_recv_t));
    if (strlen(filename) >= BT_FILENAME_LEN) {
        logger(LOG_ERROR, "filename too long");
        return -1;
    }

    tcp->next_pkt_expected = 1;
    
    tcp->p_index = p_index;
    tcp->c_index = c_index;
    tcp->ts = get_timestamp_now();
    strcpy(tcp->filename, filename);
    
    //BM_CLR(tcp->bm); // actually do nothing
    return 0;
}

/**
 * get the current ack that I should send bak
 * @param tcp, the handler
 * @return the ack
 */
static uint32_t get_ack(tcp_recv_t *tcp) {
    int i;
    
    for (i = GET_BITMAP_OFFSET(tcp, tcp->next_pkt_expected) ; i < BT_CHUNK_SIZE; i++) {
        if (!BM_ISSET(tcp->bm, i)) {
            break;
        }
    }

    // update next_pkt_expected
    if (tcp->started) {
        tcp->next_pkt_expected = REVERSE_BITMAP_OFFSET(tcp, i);
    }

    return tcp->next_pkt_expected - 1;
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
    param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = 1;
    param.ack = ack;
    param.type = PACKET_TYPE_ACK;
    
    send_packet(&param);

    return 0;
}

/**
 * receive the packet, save it into buffer
 * send corresponding ack back
 * @param tcp, the handler
 * @param pkt, the received packet
 * @return 0 on success, -1 if fails
 */
int recv_tcp(tcp_recv_t *tcp, packet_t *pkt) {
    int i;
    uint32_t offset;

    if (tcp->finished) {
        return 0;
    }
    
    /* do not update rtt until start */
    if (tcp->started && !tcp->block_update) {
        update_rtt(&tcp->rtt, &tcp->dev, tcp->ts);
    } else { /* record data_len for the first time */
        tcp->data_length = GET_DATA_LEN(pkt);
        tcp->started = 1;
        tcp->block_update = 0;
    }
    
    offset = GET_BITMAP_OFFSET(tcp, GET_SEQ(pkt));
    printf("offset: %d, len: %d\n", offset, GET_DATA_LEN(pkt));
    if (GET_SEQ(pkt) <= 0 || offset + GET_DATA_LEN(pkt) > BT_CHUNK_SIZE) {
        logger(LOG_ERROR, "data offset out of buffer");
        return -1;
    }
    
    memcpy(tcp->buffer + offset, GET_DATA(pkt), GET_DATA_LEN(pkt));

    /* set bitmap */
    for (i = 0; i < GET_DATA_LEN(pkt); i++) {
        BM_SET(tcp->bm, offset + i);
    }
    
    send_ack(tcp->p_index, get_ack(tcp));

    tcp->ts = get_timestamp_now();
    tcp->timeout_cnt = 0;

    if (offset + GET_DATA_LEN(pkt) >= BT_CHUNK_SIZE) {
        tcp->finished = 1;
    }

    return 0;
}

/**
 * handle packet loss case (timeout)
 * @param tcp
 */
static void tcp_recv_loss(tcp_recv_t *tcp) {
    int i ;
    
    for (i = 0; i < LOSS_ACK_NUM; i++) {
        send_ack(tcp->p_index, get_ack(tcp));
    }
    tcp->ts = get_timestamp_now();
    tcp->block_update = 1;
}

/**
 * check if the connection is timeout
 * @param tcp, the handler
 * @return the number of the continuous timouts, -1 if already finished
 */
int tcp_recv_timer(tcp_recv_t *tcp) {
    uint32_t now;

    /* test if finished */
    if (tcp->finished) {
        return -1;
    }
    
    now = get_timestamp_now();

    /* bootstrap for rtt */
    if ((0 == tcp->rtt) && (now - tcp->ts) < DEFAULT_TIMEOUT) {
        return tcp->timeout_cnt;
    }
    
    if ((now - tcp->ts) < GET_RTO(tcp)) {
        return tcp->timeout_cnt;
    }

    tcp->timeout_cnt++;
    tcp_recv_loss(tcp);
    
    return tcp->timeout_cnt;
}

/**
 * write the buffer to file
 * @return 0 on success -1 if fails
 */
int save_buffer(tcp_recv_t *tcp) {
    int fd;
    ssize_t ret;
    ssize_t remain = BT_CHUNK_SIZE;
    
    fd = creat(tcp->filename, 0666);
    if (fd < 0) {
        logger(LOG_ERROR, "open() error");
        return -1;
    }

WRITE_LOOP:
    while (remain > 0) {
        ret = write(fd, tcp->buffer + (BT_CHUNK_SIZE - remain), BT_CHUNK_SIZE);
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
 * @param tcp, the handler
 * @return 0 on success, 1 if check_hash fails, -1 if other IO fails
 */
int finish_tcp_recv(tcp_recv_t *tcp) {
    if (0 != check_hash()) {
        return 1;
    }

    return save_buffer(tcp);
}

/**
 * a debugging helper
 * @param tcp, the handler
 */
void dump_tcp_recv(tcp_recv_t *tcp) {
    printf("-----------------\n");
    printf("| p_index: %d\t|\n", tcp->p_index);
    printf("| c_index: %d\n", tcp->c_index);
    printf("| ts: %u\n", tcp->ts);
    printf("| rtt: %u\t|\n", tcp->rtt);
    printf("| dev: %u\t|\n", tcp->dev);
    printf("| timeout_cnt: %d\t|\n", tcp->timeout_cnt);
    printf("| filename: %s\t|\n", tcp->filename);
    printf("| started: %d\n", tcp->started);
    printf("| finished: %d\n", tcp->finished);
    printf("| next_pkt_expected : %d\n", tcp->next_pkt_expected);
    printf("| data_length: %lu\n", tcp->data_length);
}
