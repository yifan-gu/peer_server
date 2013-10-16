#include <stdio.h>

#include "packet.h"

#ifdef TESTING_PACKET
extern void test_message(uint8_t *buf, int i, ChunkList *cl);
#endif

/**
 * send packets via udp
 * @param socket, the socket fd
 * @param p, the peerlist
 * @param p_index, from which peer I will start sending
 * @param p_count, number of peers I will try to send
 * @param buf, data buffer
 * @param len, data buffer length
 */
static void send_udp(int socket, PeerList *p, int p_index, int p_count, uint8_t *buf, size_t len) {
    int i, ret;

    for (i = 0; i < p_count; i++) {
        ret = sendto(socket, buf, len, 0,
                     (struct sockaddr *) & (p->arr[p_index+i].addr), sizeof(p->arr[p_index+i].addr));
        if (ret < 0) {
            logger(LOG_ERROR, "sendto() failed");
        }
    }
}

/**
 * send packets to peers
 * @param pp, a pointer to pkt_param_t struct
 */
void send_packet(pkt_param_t *pp) {
    /* passing argumemts */
    int socket = pp->socket;
    PeerList *p = pp->p;
    int p_index = pp->p_index;
    int p_count = pp->p_count;

    ChunkList *c = pp->c;
    int c_index = pp->c_index;
    int c_count = pp->c_count;

    uint8_t type = pp->type;
    uint32_t seq = pp->seq;
    uint32_t ack = pp->ack;

    uint8_t *payload = pp->payload;;
    size_t payload_size = pp->payload_size;

    
    packet_t pkt;
    int pkt_num = 1;
    int cnt, i, j = 0;
    size_t len;
    int nondata = 0;

    uint8_t buf[PACKET_SIZE];

    nondata = (PACKET_TYPE_WHOHAS == type
               || PACKET_TYPE_IHAVE == type
               || PACKET_TYPE_GET == type
               || PACKET_TYPE_DENIED == type);
               

    /* broadcasts */
    if (p_count < 0) {
        p_count = p->count;
    }

    if (c_count < 0) {
        c_count = c->count;
    }

    if (p_index < 0 || c_index < 0) {
        logger(LOG_ERROR, "invalid peer or chunk index %d, %d", p_index, c_index);
        return;
    }

    /* compute number of packets needed */
    if (NULL != c && nondata) { // will have chunks
        pkt_num = (c_count * SHA1_HASH_SIZE + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
    }

    for (i = 0; i < pkt_num; i++) {

        /* set some common header fields */
        SET_MAGIC(&pkt, MAGIC);
        SET_VERSION(&pkt, VERSION);
        SET_TYPE(&pkt, type);
        SET_HDR_LEN(&pkt, HEADER_SIZE);
        SET_SEQ(&pkt, 0);
        SET_ACK(&pkt, 0);

        SET_PKT_LEN(&pkt, HEADER_SIZE);

        /* set seq or ack */
        if (PACKET_TYPE_DATA == type) {
            SET_SEQ(&pkt, seq);
        }

        if (PACKET_TYPE_ACK == type) {
            SET_ACK(&pkt, ack);
        }

        if (NULL != c && nondata) { // types will have chunks
            
            SET_PKT_LEN(&pkt, GET_PKT_LEN(&pkt) + 4); // 4 bytes for hash count
            
            /* iterate on chunk to add chunk hash, and update pkt_len */
            for (cnt = 0; j < MIN(c_count, HASH_NUM_PKT*(i+1)); j++, cnt++) {
                SET_HASH(&pkt, cnt, c->chunks[c_index+j].sha1);
            }

            /* update packet length and chunk counts */
            SET_PKT_LEN(&pkt, GET_PKT_LEN(&pkt) + SHA1_HASH_SIZE * cnt);
            SET_CHUNK_CNT(&pkt, cnt);

            /* set payload data */
        } else if (PACKET_TYPE_DATA == type && NULL != payload) {
            
            if (payload_size > MAX_PAYLOAD_SIZE) {
                logger(LOG_ERROR, "payload size too big %lu", payload_size);
                return;
            }
        
            memcpy(pkt.payload, payload, payload_size);
            SET_PKT_LEN(&pkt, HEADER_SIZE+payload_size);
        }

        len = GET_PKT_LEN(&pkt);
        ENCODE_PKT(buf, &pkt, GET_PKT_LEN(&pkt));

        #ifdef TESTING_PACKET
        test_message(buf, i, c);
        #endif

        send_udp(socket, p, p_index, p_count, buf, len);
    }
}

/**
 * a helper for debugging
 */
void print_packet(packet_t *pkt) {
    int i, type;
    int nondata;
    char hex[SHA1_HASH_STR_SIZE+1];

    type = GET_TYPE(pkt);
    
    nondata = (PACKET_TYPE_WHOHAS == type
               || PACKET_TYPE_IHAVE == type
               || PACKET_TYPE_GET == type
               || PACKET_TYPE_DENIED == type);
    
    printf("-----------------\n");
    printf("Magic: %d\t|\n", GET_MAGIC(pkt));
    printf("Version: %d\t|\n", GET_VERSION(pkt));
    printf("Type: %d\t\t|\n", GET_TYPE(pkt));
    printf("Hdr_len: %d\t|\n", GET_HDR_LEN(pkt));
    printf("Pkt_len: %d\t|\n", GET_PKT_LEN(pkt));
    printf("Seq: %d\t\t|\n", GET_SEQ(pkt));
    printf("Ack: %d\t\t|\n", GET_ACK(pkt));

    if (nondata) {
        printf("Chunk_cnt: %d\t|\n", GET_CHUNK_CNT(pkt));
    }
    printf("-----------------\n");

    if (nondata) {
        for (i = 0; i < GET_CHUNK_CNT(pkt); i++) {
            GET_HASH(pkt, i, hex);
            printf("%d %s\n", i, hex);
        }
    } else {
        if (type == PACKET_TYPE_DATA) {
            printf("payload: %s\n", GET_PAYLOAD(pkt));
        }
    }
    
    printf("\n");
}
