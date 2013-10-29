#include "tcp_recv.h"
#include "tcp_util.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#define BM_ISSET(bm, i) (1)
#define BM_SET(bm, i) (1)

extern int sock;
extern PeerList peerlist;

int init_tcp_recv(tcp_recv_t *tcp, const char *filename) {
    memset(tcp, 0, sizeof(tcp_recv_t));
    if (strlen(filename) >= BT_FILENAME_LEN) {
        logger(LOG_ERROR, "filename too long");
        return -1;
    }
    
    strcpy(tcp->filename, filename);
    //BM_CLR(&tcp->bm); // actually do nothing
    return 0;
}


static uint32_t get_ack(tcp_recv_t *tcp) {
    int i;
    
    for (i = GET_BITMAP_OFFSET(tcp, tcp->next_pkt_expected) ; i < BT_CHUNK_SIZE; i++) {
        if (!BM_ISSET(tcp->bm, i)) {
            break;
        } else { // update next_pkt_expected
            tcp->next_pkt_expected = REVERSE_BITMAP_OFFSET(tcp, i);
        }
    }

    return tcp->next_pkt_expected - 1;
}

/**
 * send an ack packet to peer[p_index]
 * @param p_index, the index of the pper
 * @param ack, the ack number
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

int recv_tcp(tcp_recv_t *tcp, packet_t *pkt) {
    int i;
    uint32_t offset;
    
    /* record data_len for the first time */
    if (0 == tcp->started) {
        tcp->data_length = GET_DATA_LEN(pkt);
        tcp->started = 1;
    }
    
    offset = GET_BITMAP_OFFSET(tcp, GET_SEQ(pkt));
    if (offset + GET_DATA_LEN(pkt) >= BT_CHUNK_SIZE) {
        logger(LOG_ERROR, "data offset out of buffer");
        return -1;
    }
    
    memcpy(tcp->buffer + offset, GET_DATA(pkt), GET_DATA_LEN(pkt));

    /* set bitmap */
    for (i = 0; i < GET_DATA_LEN(pkt); i++) {
        BM_SET(&tcp->bm, offset + i);
    }
    
    send_ack(tcp->util.p_index, get_ack(tcp));

    update_rtt(&tcp->util);
    tcp->util.ts = get_timestamp_now();
    tcp->util.timeout_cnt = 0;

    return 0;
}

static void tcp_recv_loss(tcp_recv_t *tcp) {
    int i ;
    
    for (i = 0; i < LOSS_ACK_NUM; i++) {
        send_ack(tcp->util.p_index, get_ack(tcp));
    }
}

/**
 * check if the connection is timeout
 */
int tcp_recv_timer(tcp_recv_t *tcp) {
    uint32_t now;

    now = get_timestamp_now();

    if (is_timeout(&tcp->util, now)) {
        tcp_recv_loss(tcp);
    }
    
    return tcp->util.timeout_cnt;
}

/**
 * write the buffer to file
 */
int save_buffer(tcp_recv_t *tcp) {
    int fd;
    ssize_t ret;
    ssize_t remain = BT_CHUNK_SIZE;

    fd = open(tcp->filename, O_CREAT);
    if (fd < 0) {
        logger(LOG_ERROR, "open() error");
        return -1;
    }

READ_LOOP:
    while (remain > 0) {
        ret = write(fd, tcp->buffer + (BT_CHUNK_SIZE - remain), BT_CHUNK_SIZE);
        if (ret < 0) {
            if (EINTR == ret) {
                remain = BT_CHUNK_SIZE;
                goto READ_LOOP;
            } else {
                logger(LOG_ERROR, "write() error");
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

int check_hash() {
    return 0;
}

int finish_tcp_recv(tcp_recv_t *tcp) {
    if (0 != check_hash()) {
        return 1;
    }

    return save_buffer(tcp);
}
