#ifndef _TCP_SEND_H_
#define _TCP_SEND_H_

#include "packet.h"
#include "tcp_util.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * the slow-start threshold size
 */
#define SS_THRESH 64
#define MAX_DUP_ACK 3

/**
 * return the offset(bytes) of the chunk in the original file
 * @param c_index, the chunk index in the "chunklist"
 */
#define GET_CHUNK_OFFSET(c_index, chunklist) (chunklist.chunks[(c_index)].id * BT_CHUNK_SIZE)


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
     * the index of the chunk I am transferring
     */
    int c_index;
    
    /**
     * store the ack counts even for obsolete ack,
     * because that is a sign of congestion
     */
    int ack_cnt[BT_CHUNK_SIZE / 1024 + 1];
    
    /**
     * array for saving timeout info for each pkt
     */
    uint32_t ack_timeout[BT_CHUNK_SIZE / 1024 + 1];

    uint32_t seq_ts[BT_CHUNK_SIZE / 1024 + 1];

    /**
     * if stop flag == 1, then bypass the send
     * if stop flag == 0, then send
     */
    int stop_flag;

    /**
     * if block update_rtt on receiving ack
     */
    int block_update;
    /**
     * if sending is finished
     */
    int finished;
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
     * deviation for computing rto
     */
    uint32_t dev;
    /**
     * the last ts when sending data
     */
    uint32_t ts;

    uint32_t last_ack_ts;
    
    int timeout_cnt;
    
    /**
     * parameters for the sender
     */
    uint32_t last_pkt_acked;
    uint32_t last_pkt_sent;
    uint32_t max_pkt_sent;
    
    //uint32_t last_pkt_sent;
    
    /**
     * the data of the file
     */
    uint8_t *data;

    /**
     * the timestamp for last updating window size in Congestion Avoidance
     */
    uint32_t ca_ts;

    uint32_t ss_threshold;
} tcp_send_t;

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

/**
 * handle the ack
 */
void tcp_handle_ack(tcp_send_t *tcp, uint32_t ack);

/**
 * update windowsize in Congestion Control state.
 * Check if one tcp send connection is timeout
 * @return the number of the continuous timouts
 */
int tcp_send_timer(tcp_send_t *tcp);

/**
 * a handy helper...
 */
void dump_tcp_send(tcp_send_t *tcp);


#endif /* _TCP_SEND_H_ */
