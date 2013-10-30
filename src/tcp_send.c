#include "tcp_send.h"
#include "packet.h"
#include "bt_parse.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

extern int sock;
extern bt_config_t config;
extern ChunkList haschunks;
extern PeerList peerlist;
extern FILE *master_chunk;

/**
 * get the data from chunk file
 * @param tcp, the tcp_send_t struct
 * @return 0 on success, -1 if fails
 */
static int load_data(tcp_send_t *tcp) {
    uint8_t *addr;
    
    /* mmap */
    addr =  mmap(NULL, BT_CHUNK_SIZE, PROT_READ, MAP_SHARED,
                 fileno(master_chunk), GET_CHUNK_OFFSET(tcp->c_index, haschunks));
    if (MAP_FAILED == addr) {
        logger(LOG_ERROR, "mmap() failed");
        perror("");
        return -1;
    }
    tcp->data = addr;
    
    return 0;
}

/**
 * a wrapper for munmap()
 */
static int unload_data(tcp_send_t *tcp) {
    if (munmap(tcp->data, BT_CHUNK_SIZE) < 0) {
        logger(LOG_ERROR, "munmap() failed");
        return -1;
    }

    return 0;
}

/**
 * send the packet within the window
 * @param tcp, the tcp_send_t struct
 * @return 0 on success, -1 if fails
 */
int send_tcp(tcp_send_t *tcp) {
    pkt_param_t param;
    uint32_t timeout;

    if (tcp->stop_flag || tcp->finished) {
        return 0;
    }
    
    /* make a packet */
    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    param.p = &peerlist;
    param.p_index = tcp->p_index;
    param.p_count = 1;
    param.type = PACKET_TYPE_DATA;
    
    for (; tcp->last_pkt_sent < (tcp->last_pkt_acked + tcp->window_size); tcp->last_pkt_sent++) {
        param.seq = tcp->last_pkt_sent + 1; //seq = the NextPacketExpected by the receiver
        
        if (param.seq > BT_CHUNK_SIZE / 1024)
            break; // reach the tail
        
        param.payload = tcp->data + ((param.seq - 1) * PAYLOAD_SIZE); // seq starts from 1, but offset starts from 0
        param.payload_size = PAYLOAD_SIZE;

        send_packet(&param);
        
        /* update timeout */
        timeout = GET_RTO(tcp);
        if (0 == timeout) { // bootstrap
            timeout = DEFAULT_TIMEOUT;
        }
        tcp->ack_timeout[param.seq] = get_timestamp_now() + timeout;
        
        tcp->ts = get_timestamp_now();
        if (tcp->ts == 0) {
            logger(LOG_ERROR, "update ts failed");
            return -1;
        }
    }

    tcp->stop_flag = 1;
    tcp->max_pkt_sent = MAX(tcp->max_pkt_sent, tcp->last_pkt_sent);

    return 0;
}

/**
 * init the tcp_send_t struct
 */
int init_tcp_send(tcp_send_t *tcp, int p_index, int c_index) {
    memset(tcp, 0, sizeof(tcp_send_t));

    tcp->window_size = 8; // TODO: change to 1 for ss
    tcp->p_index = p_index;
    tcp->c_index = c_index;
    tcp->ss_threshold = SS_THRESH;

    return load_data(tcp);
}

/**
 * deinit the tcp_send_t struct
 */
int deinit_tcp_send(tcp_send_t *tcp) {
    return unload_data(tcp);
}

/**
 * tcp send loss, either timeout or dup-ack
 */
static void tcp_send_loss(tcp_send_t *tcp) {
    tcp->ss_threshold = MAX(tcp->window_size/2, 2);
    tcp->window_size = 1;
    tcp->last_pkt_sent = tcp->last_pkt_acked;
    tcp->status = TCP_STATUS_FAST_RETRANSMIT;
    tcp->block_update = 1;
    tcp->stop_flag = 0;

    return;
}

/**
 * handle the ack
 */
void tcp_handle_ack(tcp_send_t *tcp, uint32_t ack) {
    if (tcp->finished) {
        return;
    }
    
    if ((ack > tcp->max_pkt_sent) || (ack <= 0)) {
        logger(LOG_INFO, "invalid ack number");
        return;
    }

    if (!tcp->block_update) { // do not update rtt in first ack from FR
        update_rtt(&tcp->rtt, &tcp->dev, tcp->ts);
    } else {
        tcp->block_update = 0;
    }

    if (++tcp->ack_cnt[ack] > MAX_DUP_ACK) { // test if dup ack
        tcp->ack_cnt[ack] = 0;
        tcp_send_loss(tcp);
        return;
    }

    if (ack > tcp->last_pkt_acked) { // update last_ack and window size
        switch (tcp->status) {
        case TCP_STATUS_SLOW_START:
            tcp->window_size += ack - tcp->last_pkt_acked;
            if (tcp->window_size > tcp->ss_threshold) {
                tcp->window_size = tcp->ss_threshold;
                tcp->status = TCP_STATUS_CONGESTION_AVOIDANCE;
            }
            break;

        case TCP_STATUS_CONGESTION_AVOIDANCE: // update rtt, but not here
            break;
        case TCP_STATUS_FAST_RETRANSMIT:
            if (ack >= tcp->max_pkt_sent) { // FR finished
                tcp->status = TCP_STATUS_SLOW_START;
            } else if (tcp->window_size > tcp->ss_threshold) {
                tcp->window_size = tcp->ss_threshold;
                tcp->status = TCP_STATUS_CONGESTION_AVOIDANCE;
            } else {
                tcp->window_size++;
            }
            
            tcp->last_pkt_sent = ack; // do not retransmit those already received packets
        }
        
        tcp->stop_flag = 0; // now should be able to send
        tcp->timeout_cnt = 0; // clear continuous timeouts
        tcp->last_pkt_acked = ack;
        
        if (tcp->last_pkt_acked >= BT_CHUNK_SIZE / 1024) {
            tcp->finished = 1;
        }
    }

    return;
}

/**
 * update windowsize in Congestion Control state.
 * Check if one tcp send connection is timeout
 * @return the number of the continuous timouts, -1 if already finished
 */
int tcp_send_timer(tcp_send_t *tcp) {
    uint32_t now;

    /* test if finished */
    if (tcp->finished) {
        return -1;
    }
    
    now = get_timestamp_now();
    
    /* bootstrap for updating window size in Congestion Avoidance */
    if ((0 == tcp->ca_ts) && (TCP_STATUS_CONGESTION_AVOIDANCE == tcp->status)) {
        tcp->ca_ts = now;
    }

    /* increase window size each RTT in Congestion Avoidance */
    if ((TCP_STATUS_CONGESTION_AVOIDANCE == tcp->status)
        && (now - tcp->ca_ts > tcp->rtt)) {
        tcp->window_size ++;
        tcp->ca_ts = now;
    }
    
    if (0 == tcp->stop_flag) { // nothing sent, so no need to check timeouts
        return tcp->timeout_cnt;
    }

    if (now < tcp->ack_timeout[tcp->last_pkt_acked+1]) {
        return tcp->timeout_cnt;
    }

    tcp->timeout_cnt++;
    tcp_send_loss(tcp);
    
    return tcp->timeout_cnt;
}

/**
 * a handy helper...
 */
void dump_tcp_send(tcp_send_t *tcp) {
    int i;
    
    printf("-----------------\n");
    printf("| stop_flag: %d\t|\n", tcp->stop_flag);
    printf("| finished: %d\t|\n", tcp->finished);
    printf("| p_index: %d\t|\n", tcp->p_index);
    switch(tcp->status) {
    case TCP_STATUS_SLOW_START:
        printf("| status: SS\t|\n");
        break;
    case TCP_STATUS_CONGESTION_AVOIDANCE:
        printf("| status: CA\t|\n");
        break;
    case TCP_STATUS_FAST_RETRANSMIT:
        printf("| status: FR\t|\n");
        break;
    case TCP_STATUS_FAST_RECOVERY:
        printf("| status: FC\t|\n");
        break;
    default:
        printf("| status: undefined\t\n");
    }
    printf("| window_size: %d\t|\n", tcp->window_size);
    printf("| rtt: %u\t|\n", tcp->rtt);
    printf("| dev: %u\t|\n", tcp->dev);
    printf("| last_ack: %d\t|\n", tcp->last_pkt_acked);
    printf("| last_sent: %d\t\n", tcp->last_pkt_sent);
    printf("| max_sent: %d\t\n", tcp->max_pkt_sent);
    printf("| timeout_cnt: %d\n", tcp->timeout_cnt);
    printf("| c_index: %d\n", tcp->c_index);
    printf("| block_update: %d\t\n", tcp->block_update);
    printf("| data: %p\n", tcp->data);
    printf("| ts: %u\n", tcp->ts);
    printf("| ss_threshold: %d\n", tcp->ss_threshold);

    printf("dup acks:\n");
    for (i = 0; i < (BT_CHUNK_SIZE / 1024 + 1); i++) {
        if (tcp->ack_cnt[i] > 1) {
            printf("%d, ", i);
        }
    }
    printf("\n");

    return;
}
