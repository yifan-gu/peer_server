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
        
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/**
 * update the round-trip time, and deviation
 */
void update_rtt(tcp_util_t *util) {
    uint32_t s_rtt;
    uint32_t s_dev;

    s_rtt = get_timestamp_now() - util->ts;
    if (s_rtt < 0) {
        logger(LOG_ERROR, "get_timestamp_now() failed");
        return;
    }
    
    if (0 == (util->rtt)) { // first rtt
        util->rtt = s_rtt;
    } else {
        util->rtt = util->rtt * 7 / 8 + s_rtt / 8;
    }

    s_dev = (s_rtt > util->rtt) ? (s_rtt - util->rtt) : (util->rtt - s_rtt);
    if (0 == util->dev) { // first dev
        util->dev = s_dev;
    } else {
        util->dev = util->dev * 3 / 4 + s_dev / 4;
    }

    return;
}

/**
 * check if timeout
 * @return 1 if timeout, 0 if not
 */
int is_timeout(tcp_util_t *util, uint32_t now) {
    /* bootstrap for rtt */
    if ((0 == util->rtt) && (now - util->ts) < DEFAULT_TIMEOUT) {
        return 0;
    }
    
    if ((now - util->ts) < GET_RTO(util)) {
        return 0;
    }

    util->timeout_cnt++;
    
    return 1;
}
