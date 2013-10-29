#ifndef _TCP_UTIL_H
#define _TCP_UTIL_H

#include <inttypes.h>

#define DEFAULT_TIMEOUT (10 * 1000) // milliseconds

typedef struct tcp_util_s {
    
    /**
     * the index of the peer I am communicating with
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
    
    /**
     * variables for handling loss
     */
    int timeout_cnt;
} tcp_util_t;


#define GET_RTO(util) ((util)->rtt + 4 * (util)->dev)

/**
 * update the round-trip time, and deviation
 */
void update_rtt(tcp_util_t *util);

/**
 * get current timestamp
 */
uint32_t get_timestamp_now();

/**
 * check if timeout
 * @return 1 if timeout, 0 if not
 */
int is_timeout(tcp_util_t *util, uint32_t now);

#endif
