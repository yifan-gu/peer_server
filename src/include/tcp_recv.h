#ifndef _TCP_SEND_H_
#define _TCP_SEND_H_

#include "packet.h"
#include "tcp_util.h"
#include "bitmap.h"

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

    /**
     * the peer index
     */
    int p_index;
    /**
     * the index of the chunk I am transferring
     */
    int c_index;
    
    uint32_t rtt;
    uint32_t dev;

    /**
     * the last sending data
     */
    uint32_t ts;
    
    int timeout_cnt;
    
    char filename[BT_FILENAME_LEN];
    /**
     * whether the downloading is started
     */
    int started;

    int block_update;
    /**
     * whether the downloading is finished
     */
    int finished;
    /**
     * parameters for the receiver
     */
    uint32_t next_pkt_expected;
    
    /**
     * the recv buffer
     */
    char buffer[BT_CHUNK_SIZE];

    size_t data_length;
    
    Recvbm bm;
} tcp_recv_t;

/**
 * init the receiver handler
 * @param tcp, the handler
 * @param p_index, the peer index
 * @param c_index, the chunk index
 * @param filename, the target filename
 * @return 0 on success, -1 if fails
 */
int init_tcp_recv(tcp_recv_t *tcp, int p_index, int c_index, const char *filename);

/**
 * receive the packet, save it into buffer
 * send corresponding ack back
 * @param tcp, the handler
 * @param pkt, the received packet
 * @return 0 on success, -1 if fails
 */
int recv_tcp(tcp_recv_t *tcp, packet_t *pkt);

/**
 * check if the connection is timeout
 * @param tcp, the handler
 * @return 0 on success, -1 if fails
 */
int tcp_recv_timer(tcp_recv_t *tcp);

/**
 * a wrapper for finishing the downloading
 * @param tcp, the handler
 * @return 0 on success, 1 if check_hash fails, -1 if other IO fails
 */
int finish_tcp_recv(tcp_recv_t *tcp);

/**
 * write the buffer to file
 * @return 0 on success -1 if fails
 */
int save_buffer(tcp_recv_t *tcp);

/**
 * a debugging helper
 * @param tcp, the handler
 */
void dump_tcp_recv(tcp_recv_t *tcp);

#endif /* _TCP_RECV_H */
