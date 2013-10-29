#ifndef _TCP_SEND_H_
#define _TCP_SEND_H_

#include "packet.h"
#include "tcp_util.h"

/**
 * the max receiver's window size
 */
#define RECV_WINDOW_SIZE 512

/**
 * number of acks to send when timeouts
 */
#define LOSS_ACK_NUM 3

#define GET_BITMAP_OFFSET(tcp, seq) ((tcp)->data_length * (seq - 1))

#define REVERSE_BITMAP_OFFSET(tcp, i) ((i) / (tcp)->data_length + 1)

/**
 * assuming data size does not change during one connection
 */
typedef struct tcp_recv_s {

    tcp_util_t util;
    
    /**
     * the peer index
     */

    char filename[BT_FILENAME_LEN];
    /**
     * whether the downloading has started
     */
    int started;
    /**
     * parameters for the receiver
     */
    uint32_t next_pkt_expected;
    
    /**
     * the buffer for the receiver
     */
    packet_t recv_buf[RECV_WINDOW_SIZE];

    /**
     * the recv buffer
     */
    char buffer[BT_CHUNK_SIZE];

    size_t data_length;
    
    Recvbm bm;
} tcp_recv_t;

/**
 * init the tcp_recv_t struct
 */
int init_tcp_recv(tcp_recv_t *tcp, const char *filename);

/**
 * receive the packet, save it into buffer
 * return the accumulative ack
 */
int recv_tcp(tcp_recv_t *tcp, packet_t *pkt);

/**
 * check if the connection is timeout
 */
int tcp_recv_timer(tcp_recv_t *tcp);

/**
 * test hash and write to file
 */
int save_buffer(tcp_recv_t *tcp);


#endif /* _TCP_RECV_H */
