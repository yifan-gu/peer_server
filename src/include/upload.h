#ifndef _UPLOAD_H
#define _UPLOAD_H

#include "packet.h"
#include "util.h"

/**
 * the slow-start threshold size
 */
#define SS_THRESH 64
#define MAX_DUP_ACK 3
#define WINDOW_FILE "problem2-peer.txt"

/**
 * return the offset(bytes) of the chunk in the original file
 * @param c_index, the chunk index in the "chunklist"
 */
#define GET_CHUNK_OFFSET(c_index, chunklist) (chunklist.chunks[(c_index)].id * BT_CHUNK_SIZE)

#define MAX_DUP_SEQ(ul, ack) MIN(0, 0 - ((int)(ack) - ((int)(ul)->last_pkt_acked + 1)))

enum ul_status {
    UL_STATUS_SLOW_START = 0,
    UL_STATUS_CONGESTION_AVOIDANCE = 1,
    UL_STATUS_FAST_RETRANSMIT = 2,
    UL_STATUS_FAST_RETRANSMIT_SS = 3,
    UL_STATUS_FAST_RETRANSMIT_CA = 4,
    UL_STATUS_FAST_RECOVERY = 5,
};

typedef struct upload_s {
    /**
     * The chunk index in haschunks
     */
    int has_index;
    int p_index;

    /**
     * duplicate acks: last ack number and counts
     */
    int ack_cnt[BT_CHUNK_SIZE / 1024 + 1];
    
    /**
     * last sent data timestamp
     */
    //uint32_t ts;
    uint32_t seq_ts[BT_CHUNK_SIZE / 1024 + 1];
    uint32_t seq_timeout[BT_CHUNK_SIZE / 1024 + 1];
    
    /**
     * congestion control:
     * - ssthresh
     * - last ack number
     * - current window size
     * - rtt
     */
    uint32_t ss_threshold;
    uint32_t last_pkt_acked;
    uint32_t last_pkt_sent;
    uint32_t max_pkt_sent;
    uint32_t max_retransmit_seq;
    uint32_t rtt;
    uint32_t dev;
    uint32_t rto;
    uint32_t window_size;
    
    /**
     * the time to update window size
     */
    uint32_t ca_ts;
    int timeout_cnt;

    /**
     * data
     */
    uint8_t *data;

    /**
     * status and flags
     */
    int status;
    int stop_flag;
    int finished;
}upload_t;

/**
 * alias
 */
typedef upload_t Upload;

/**
 * send the packet within the window
 * @param ul, the upload_t struct
 * @return 0 on success, -1 if fails
 */
int ul_send(upload_t *ul);

/**
 * init the upload_t struct
 */
int ul_init(upload_t *ul, int p_index, int has_index);

/**
 * deinit the upload_t struct
 */
int ul_deinit(upload_t *ul);

/**
 * handle the ack
 */
void ul_handle_ack(upload_t *ul, uint32_t ack);

/**
 * ul_check_timeout
 * @return Number of continuous timeout times. -1 if the connection is finished
 */
int ul_check_timeout(upload_t *ul);

/**
 * write the window size to file
 */
int ul_write_winsize(upload_t *ul);

/**
 * a handy helper...
 */
void ul_dump(upload_t *ul, FILE *fp);

#endif // for #ifndef _UPLOAD_H
