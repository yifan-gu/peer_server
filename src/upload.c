#include "upload.h"
#include "packet.h"
#include "bt_parse.h"
#include "logger.h"
#include "peer_server.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

extern PeerServer psvr;
//extern FILE *log_fp;

/**
 * get the data from chunk file
 * @param ul, the upload_t struct
 * @return 0 on success, -1 if fails
 */
static int load_data(upload_t *ul) {
    uint8_t *addr;
    
    /* mmap */
    addr =  mmap(NULL, BT_CHUNK_SIZE, PROT_READ, MAP_SHARED,
                 fileno(psvr.master_chunk),
                 GET_CHUNK_OFFSET(ul->has_index, psvr.haschunks));
    if (MAP_FAILED == addr) {
        logger(LOG_ERROR, "mmap() failed");
        perror("");
        return -1;
    }
    ul->data = addr;
    
    return 0;
}

/**
 * a wrapper for munmap()
 */
static int unload_data(upload_t *ul) {
    if (munmap(ul->data, BT_CHUNK_SIZE) < 0) {
        logger(LOG_ERROR, "munmap() failed");
        return -1;
    }

    return 0;
}

/**
 * send the packet within the window
 * @param ul, the upload_t struct
 * @return 0 on success, -1 if fails
 */
int ul_send(upload_t *ul) {
    pkt_param_t param;

    /*if (ul->stop_flag || ul->finished) {
        return 0;
        }*/
    
    /* make a packet */
    PKT_PARAM_CLEAR(&param);
    param.socket = psvr.sock;
    //param.p = &peerlist;
    param.p_index = ul->p_index;
    param.p_count = 1;
    param.type = PACKET_TYPE_DATA;
    
    for (; ul->last_pkt_sent < (ul->last_pkt_acked + ul->window_size); ul->last_pkt_sent++) {
        param.seq = ul->last_pkt_sent + 1; //seq = the NextPacketExpected by the receiver
        
        if (param.seq > BT_CHUNK_SIZE / PAYLOAD_SIZE)
            break; // reach the tail
        
        param.payload = ul->data + ((param.seq - 1) * PAYLOAD_SIZE); // seq starts from 1, but offset starts from 0
        param.payload_size = PAYLOAD_SIZE;

        send_packet(&param);
        
        logger(LOG_DEBUG, "send seq: %d", param.seq);

         /* update timeout */
        ul->rto = GET_RTO(ul);
        if (0 == ul->rto) { // bootstrap
            ul->rto = DEFAULT_TIMEOUT;
        }
        
        ul->seq_ts[param.seq] = get_timestamp_now();
        if (ul->seq_ts[param.seq] == 0) {
            logger(LOG_ERROR, "update ts failed");
            return -1;
        }
        ul->seq_timeout[param.seq] = ul->seq_ts[param.seq] + ul->rto;
    }

    //ul->stop_flag = 1;
    ul->max_pkt_sent = MAX(ul->max_pkt_sent, ul->last_pkt_sent);

    return 0;
}

/**
 * init the upload_t struct
 */
int ul_init(upload_t *ul, int p_index, int has_index) {
    memset(ul, 0, sizeof(upload_t));
    ul->window_size = 1;
    ul->p_index = p_index;
    ul->has_index = has_index;
    ul->ss_threshold = SS_THRESH;
    ul->rtt = DEFAULT_TIMEOUT / 2;
    ul->rto = DEFAULT_TIMEOUT;

    write_winsize(ul->p_index, ul->window_size);
    
    return load_data(ul);
}


/**
 * deinit the upload_t struct
 */
int ul_deinit(upload_t *ul) {
    return unload_data(ul);
}

/**
 * ul send loss, either timeout or dup-ack
 */
static void ul_loss(upload_t *ul) {
    ul->ss_threshold = MAX(ul->window_size/2, 2);
    ul->window_size = 1;
    ul->last_pkt_sent = ul->last_pkt_acked;
    ul->status = UL_STATUS_FAST_RETRANSMIT;
    ul->max_retransmit_seq = ul->max_pkt_sent;
    //ul->stop_flag = 0;

    write_winsize(ul->p_index, ul->window_size);

    return;
}

/**
 * handle the ack
 */
void ul_handle_ack(upload_t *ul, uint32_t ack) {
    if (ul->finished) {
        return;
    }
    // if ack == 0, just restart
    if (ack <= 0) {
        //ul->stop_flag = 0;
        ul->timeout_cnt = 0;
        return;
    }
    
    if ((ack > ul->max_pkt_sent) || (ack < ul->last_pkt_acked)) {
        logger(LOG_INFO, "invalid ack number");
        return;
    }

    // update RTT only in SS and CA
    if ((UL_STATUS_SLOW_START == ul->status)
        || (UL_STATUS_CONGESTION_AVOIDANCE == ul->status)) {
        update_rtt(&ul->rtt, &ul->dev, ul->seq_ts[ack]);
    }

    // take the dup SEQ into account when in retransmission
    if ((UL_STATUS_FAST_RETRANSMIT_SS == ul->status)
        || (UL_STATUS_FAST_RETRANSMIT_CA == ul->status)) {
        ul->ack_cnt[ack] = MAX_DUP_SEQ(ul, ack); 
    }

    // only count dup ack for NON-FR state
    if ((UL_STATUS_FAST_RETRANSMIT != ul->status) 
        && (++ul->ack_cnt[ack] > MAX_DUP_ACK)) { // test if dup ack
            ul->ack_cnt[ack] = 0;
            ul_loss(ul);
            return;
    }

    if (ack > ul->last_pkt_acked) { // update last_ack and window size
        switch (ul->status) {
        case UL_STATUS_SLOW_START:
            ul->window_size += ack - ul->last_pkt_acked;
            if (ul->window_size > ul->ss_threshold) {
                ul->status = UL_STATUS_CONGESTION_AVOIDANCE;
            }
            write_winsize(ul->p_index, ul->window_size);
            break;

        case UL_STATUS_CONGESTION_AVOIDANCE: // update rtt, but not here
            break;
            
        case UL_STATUS_FAST_RETRANSMIT:
            if (ack >= ul->last_pkt_sent) { // change state to SS after a successful transmission
                ul->status = UL_STATUS_FAST_RETRANSMIT_SS;
                ul->window_size++;
                write_winsize(ul->p_index, ul->window_size);
            }
            break;

            /*
             * since SS in retransmission is different with real SS, we made it
             * a different state, the same thing happen with CA
             */
        case UL_STATUS_FAST_RETRANSMIT_SS:
            ul->window_size++;
            write_winsize(ul->p_index, ul->window_size);
            if (ack >= ul->max_retransmit_seq) { // exit FR state
                ul->status = UL_STATUS_SLOW_START;
                break;
            }
            
            if (ul->window_size > ul->ss_threshold) {
                ul->status = UL_STATUS_FAST_RETRANSMIT_CA;
            }
            
            break;

        case UL_STATUS_FAST_RETRANSMIT_CA:
            if (ack >= ul->max_retransmit_seq) { // exit FR state
                ul->status = UL_STATUS_CONGESTION_AVOIDANCE;
            }
            break;
        }

        if (ul->last_pkt_sent < ack) { // do not retransmit those already received packets
            ul->last_pkt_sent = ack;
        }
        
        //ul->stop_flag = 0; // now should be able to send
        ul->timeout_cnt = 0; // clear continuous timeouts
        ul->last_pkt_acked = ack;
        
        if (ul->last_pkt_acked >= BT_CHUNK_SIZE / PAYLOAD_SIZE) {
            ul->finished = 1;
        }
    }

    return;
}

/**
 * update windowsize in Congestion Control state.
 * Check if one ul send connection is timeout
 * @return the number of the continuous timouts, -1 if already finished
 */
int ul_check_timeout(upload_t *ul) {
    uint32_t now;

    /* test if finished */
    if (ul->finished) {
        return -1;
    }
    
    now = get_timestamp_now();
    
    /* bootstrap for updating window size in Congestion Avoidance */
    if ((0 == ul->ca_ts) &&
        ((UL_STATUS_CONGESTION_AVOIDANCE == ul->status)
         || (UL_STATUS_FAST_RETRANSMIT_CA == ul->status))) {
        ul->ca_ts = now + ul->rtt;
    }

    /* increase window size each RTT in Congestion Avoidance */
    if (((UL_STATUS_CONGESTION_AVOIDANCE == ul->status)
         || (UL_STATUS_FAST_RETRANSMIT_CA == ul->status))
        && (now > ul->ca_ts)) {
        ul->window_size++;
        write_winsize(ul->p_index, ul->window_size);
        ul->ca_ts = now + ul->rtt;
    }
    
    /*if (0 == ul->stop_flag) { // nothing sent, so no need to check timeouts
        return ul->timeout_cnt;
        }*/

    /*
     * test if timeout
     * if timeout is 0, means the timeout ts is invalid
     */
    if (0 == ul->seq_timeout[ul->last_pkt_acked + 1]
        || now < ul->seq_timeout[ul->last_pkt_acked + 1]) {
        return ul->timeout_cnt;
    }

    ul->rtt = MIN(ul->rtt*2, DEFAULT_TIMEOUT/4);
    ul->timeout_cnt++;
    ul_loss(ul);
    
    //ul_dump(ul, log_fp);
    ul_send(ul);
    return ul->timeout_cnt;
}

/**
 * a handy helper...
 */
void ul_dump(upload_t *ul, FILE *fp) {
    int i;
    
    fprintf(fp, "-----------------\n");
    //fprintf(fp, "| stop_flag: %d\t|\n", ul->stop_flag);
    fprintf(fp, "| finished: %d\t|\n", ul->finished);
    fprintf(fp, "| p_index: %d\t|\n", ul->p_index);
    switch(ul->status) {
    case UL_STATUS_SLOW_START:
        fprintf(fp, "| status: SS\t|\n");
        break;
    case UL_STATUS_CONGESTION_AVOIDANCE:
        fprintf(fp, "| status: CA\t|\n");
        break;
    case UL_STATUS_FAST_RETRANSMIT:
        fprintf(fp, "| status: FR\t|\n");
        break;
    case UL_STATUS_FAST_RECOVERY:
        fprintf(fp, "| status: FC\t|\n");
        break;
    case UL_STATUS_FAST_RETRANSMIT_SS:
        fprintf(fp, "| status: FR_SS\n");
        break;
    case UL_STATUS_FAST_RETRANSMIT_CA:
        fprintf(fp, "| status: FR_CA\n");
        break;
    default:
        fprintf(fp, "| status: undefined\t\n");
    }
    fprintf(fp, "| window_size: %d\t|\n", ul->window_size);
    fprintf(fp, "| rtt: %u\t|\n", ul->rtt);
    fprintf(fp, "| dev: %u\t|\n", ul->dev);
    fprintf(fp, "| last_ack: %d\t|\n", ul->last_pkt_acked);
    fprintf(fp, "| last_sent: %d\t\n", ul->last_pkt_sent);
    fprintf(fp, "| max_sent: %d\t\n", ul->max_pkt_sent);
    fprintf(fp, "| max_retransmit: %d\t\n", ul->max_retransmit_seq);
    fprintf(fp, "| timeout_cnt: %d\n", ul->timeout_cnt);
    fprintf(fp, "| has_index: %d\n", ul->has_index);
    fprintf(fp, "| data: %p\n", ul->data);
    fprintf(fp, "| rto: %u\n", ul->rto);
    fprintf(fp, "| ss_threshold: %d\n", ul->ss_threshold);

    fprintf(fp, "dup acks:\n");
    for (i = 0; i < (BT_CHUNK_SIZE / PAYLOAD_SIZE + 1); i++) {
        if (ul->ack_cnt[i] > 1) {
            fprintf(fp, "%d: %d, ", i, ul->ack_cnt[i]);
        }
    }
    fprintf(fp, "\n");

    return;
}
