#ifndef _TCP_UTIL_H
#define _TCP_UTIL_H

#include <inttypes.h>

#define DEFAULT_TIMEOUT (10 * 1000) // milliseconds

#define GET_RTO(tcp) (2 * (tcp)->rtt)// + 1 * (tcp)->dev)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/**
 * update the round-trip time, and deviation
 */
void update_rtt(uint32_t *rtt, uint32_t *dev, uint32_t ts);

/**
 * get current timestamp
 */
uint32_t get_timestamp_now(void);

#endif
