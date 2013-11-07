#include <stdio.h>

#include "packet.h"
#include "spiffy.h"
#include "peerlist.h"
#include <peer_server.h>

extern PeerServer psvr;

/**
 * send packets via udp
 * @param socket, the socket fd
 * @param p, the peerlist
 * @param p_index, from which peer I will start sending
 * @param p_count, number of peers I will try to send
 * @param buf, data buffer
 * @param len, data buffer length
 */
void send_udp(int socket, PeerList *p, int p_index, int p_count, uint8_t *buf, size_t len) {
    int i, ret;

    for (i = 0; i < p_count; i++) {
        ret = spiffy_sendto(socket, buf, len, 0,
                            (struct sockaddr *) & (p->peers[p_index+i].addr),
                            sizeof(p->peers[p_index+i].addr));
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
    //PeerList *p = pp->p;
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
        p_count = psvr.peerlist.count;
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

            if (PACKET_TYPE_GET == type) {
                SET_PKT_LEN(&pkt, GET_PKT_LEN(&pkt));
            } else {
                SET_PKT_LEN(&pkt, GET_PKT_LEN(&pkt) + 4); // 4 bytes for hash count
            }
            
            /* iterate on chunk to add chunk hash, and update pkt_len */
            for (cnt = 0; j < MIN(c_count, HASH_NUM_PKT*(i+1)); j++, cnt++) {
                SET_HASH(&pkt, cnt, c->chunks[c_index+j].sha1);
            }

            /* update packet length and chunk counts */
            SET_PKT_LEN(&pkt, GET_PKT_LEN(&pkt) + SHA1_HASH_SIZE * cnt);

            /* no CHUNK_CNU for GET */
            if (PACKET_TYPE_GET != type) {
                SET_CHUNK_CNT(&pkt, cnt);
            }
            

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
        ENCODE_PKT(buf, &pkt);

        send_udp(socket, &psvr.peerlist, p_index, p_count, buf, len);
    }
}

/**
 * check if a packet is valid
 * @param pkt,
 * @return 1 if valid, 0 if not
 */
int valid_packet(packet_t *pkt) {
    if ( GET_MAGIC(pkt) != MAGIC) {
        logger(LOG_INFO, "Invalid Magic");
        return 0;
    }

    if (GET_VERSION(pkt) != VERSION) {
        logger(LOG_INFO, "Invalid Version");
        return 0;
    }

    if (GET_PKT_LEN(pkt) < GET_HDR_LEN(pkt)
        || GET_HDR_LEN(pkt) < HEADER_SIZE) { // invalid header len
        logger(LOG_INFO, "Invalid Hdr Len: %d or Pkt Len: %d", GET_HDR_LEN(pkt), GET_PKT_LEN(pkt));
        return 0;
    }

    if (GET_PKT_LEN(pkt) > PACKET_SIZE) {
        logger(LOG_INFO, "Invalid Pkt Len: %d", GET_PKT_LEN(pkt));
        return 0;
    }

    switch (GET_TYPE(pkt)) {
    case PACKET_TYPE_WHOHAS:
    case PACKET_TYPE_IHAVE:
        if (GET_DATA_LEN(pkt) < 4) {
            logger(LOG_INFO, "Invalid Data Len: %d, too small!", GET_DATA_LEN(pkt));
            return 0;
        }

        if (GET_CHUNK_CNT(pkt) <= 0
            || GET_CHUNK_CNT(pkt) > (GET_DATA_LEN(pkt) - 4) / SHA1_HASH_SIZE) {
            logger(LOG_INFO, "Invalid CHUNK CNT: %d, max is: %d",
                   GET_CHUNK_CNT(pkt), (GET_DATA_LEN(pkt) - 4) / SHA1_HASH_SIZE);
            return 0;
        }
        break;

    case PACKET_TYPE_GET:
        if (GET_DATA_LEN(pkt) < SHA1_HASH_SIZE) {
            logger(LOG_INFO, "Invalid Data Len for GET: %d, too small!", GET_DATA_LEN(pkt));
            return 0;
        }
        break;
    }
    
    return 1;
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
    printf("| Magic: %d\t|\n", GET_MAGIC(pkt));
    printf("| Version: %d\t|\n", GET_VERSION(pkt));
    printf("| Type: %d\t|\n", GET_TYPE(pkt));
    printf("| Hdr_len: %d\t|\n", GET_HDR_LEN(pkt));
    printf("| Pkt_len: %4d |\n", GET_PKT_LEN(pkt));
    printf("| Seq: %u\t|\n", GET_SEQ(pkt));
    printf("| Ack: %u\t|\n", GET_ACK(pkt));
    printf("| Data_len: %4d|\n", GET_DATA_LEN(pkt));

    if (nondata && PACKET_TYPE_GET != type) {
        printf("| Chunk_cnt: %d\t|\n", GET_CHUNK_CNT(pkt));
    }
    printf("-----------------\n");

    if (nondata && PACKET_TYPE_GET != type) {
        for (i = 0; i < GET_CHUNK_CNT(pkt); i++) {
            GET_HASH(pkt, i, hex);
            printf("%d %s\n", i, hex);
        }
    } else if (PACKET_TYPE_GET == type) {
        GET_HASH(pkt, 0, hex);
        printf("%d %s\n", 0, hex);
    }
    
    printf("\n");
}
