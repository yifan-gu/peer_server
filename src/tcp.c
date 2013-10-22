#include "tcp.h"
#include "packet.h"
#include "bt_parse.h"
#include "logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
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
                 fileno(master_chunk), GET_OFFSET(tcp->c_index, haschunks));
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
    int i;
    pkt_param_t param;
    struct timeval tv;

    /* make a packet */
    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    param.p = &peerlist;
    param.p_index = tcp->p_index;
    param.p_count = 1;
    param.type = PACKET_TYPE_DATA;
    
    for (i = 0; i < tcp->window_size; i++) {
        param.seq = (tcp->last_pkt_acked + 1) + i; //seq = the NextPacketExpected by the receiver
        param.payload = tcp->data + ((param.seq - 1) * PAYLOAD_SIZE); // seq starts from 1, but offset starts from 0
        param.payload_size = PAYLOAD_SIZE;

        send_packet(&param);
        
        /* update ts */
        if (gettimeofday(&tv, NULL) < 0) {
            logger(LOG_ERROR, "gettimeofday() failed");
            return -1;
        }
        
        tcp->ts = (tv.tv_sec * 1000000) + tv.tv_usec;
    }

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


