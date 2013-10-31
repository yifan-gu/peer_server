#include "tcp_util.h"
#include "logger.h"

#include <sys/time.h>
#include <stdio.h>

/**
 * get current timestamp
 */
uint32_t get_timestamp_now() {
    struct timeval tv;
    
    if (gettimeofday(&tv, NULL) < 0) {
        logger(LOG_ERROR, "gettimeofday() failed");
            return 0;
    }
        
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000); // convert to milliseconds
}

/**
 * update the round-trip time, and deviation
 */
void update_rtt(uint32_t *rtt, uint32_t *dev, uint32_t ts) {
    uint32_t s_rtt;
    uint32_t s_dev;

    if (0 == ts) { // do nothing
        return;
    }

    s_rtt = get_timestamp_now() - ts;
    if (s_rtt < 0) {
        logger(LOG_ERROR, "get_timestamp_now() failed");
        return;
    }
    
    if (0 == ((*rtt))) { // first rtt
        (*rtt) = s_rtt;
    } else {
        (*rtt) = (*rtt) * 7 / 8 + s_rtt / 8;
    }

    s_dev = (s_rtt > (*rtt)) ? (s_rtt - (*rtt)) : ((*rtt) - s_rtt);
    if (0 == (*dev)) { // first dev
        (*dev) = s_dev;
    } else {
        (*dev) = (*dev) * 3 / 4 + s_dev / 4;
    }

    return;
}
