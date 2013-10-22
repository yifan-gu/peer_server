#ifndef _TCP_H_
#define _TCP_H_

#include "packet.h"

/**
 * the max receiver's window size
 */
#define RECV_WINDOW_SIZE 512

/**
 * the slow-start threshold size
 */
#define SS_THRESH 64

/**
 * return the offset(bytes) of the chunk in the original file
 * @param c_index, the chunk index in the "chunklist"
 */
#define GET_OFFSET(c_index, chunklist) (chunklist.chunks[(c_index)].id * BT_CHUNK_SIZE)

enum tcp_status {
    TCP_STATUS_SLOW_START,
    TCP_STATUS_CONGESTION_AVOIDANCE,
    TCP_STATUS_FAST_RETRANSMIT,
    TCP_STATUS_FAST_RECOVERY
};

typedef struct tcp_send_s {

    /**
     * the index of the peer I am communicating with
     */
    int p_index;
    /**
     * status of the sender
     */
    uint8_t status;
    
    /**
     * the sender's window size
     */
    uint32_t window_size;

    /**
     * round-trip time, in millisecond
     */
    uint32_t rtt;
    
    /**
     * parameters for the sender
     */
    uint32_t last_pkt_acked;
    //uint32_t last_pkt_sent;
    
    /**
     * the index of the chunk I am transferring
     */
    int c_index;
    
    /**
     * the data of the file
     */
    uint8_t *data;

    /**
     * the last sending data
     */
    uint32_t ts;

    uint32_t ss_threshold;
    
} tcp_send_t;

typedef struct tcp_recv_s {
    /**
     * parameters for the receiver
     */
    uint32_t next_pkt_expected;
    uint32_t last_pkt_read;

    /**
     * the buffer for the receiver
     */
    packet_t recv_buf[RECV_WINDOW_SIZE];

    /**
     * the last time sending ack
     */
    uint32_t ts;
    
} tcp_recv_t;

/**
 * send the packet within the window
 * @param tcp, the tcp_send_t struct
 * @return 0 on success, -1 if fails
 */
int send_tcp(tcp_send_t *tcp);

/**
 * init the tcp_send_t struct
 */
int init_tcp_send(tcp_send_t *tcp, int p_index, int c_index);

/**
 * deinit the tcp_send_t struct
 */
int deinit_tcp_send(tcp_send_t *tcp);

#endif /* _TCP_H_ */
