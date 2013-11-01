#ifndef _UTIL_H
#define _UTIL_H

#include <inttypes.h>

#define DEFAULT_TIMEOUT (10 * 1000) // milliseconds

#define GET_RTO(ptr) ((ptr)->rtt + 4 * (ptr)->dev)

/**
 * update the round-trip time, and deviation
 */
void update_rtt(uint32_t *rtt, uint32_t *dev, uint32_t ts);

/**
 * get current timestamp
 */
uint32_t get_timestamp_now();

#endif
