/*
 * A simple implementation for packet I/O
 * @author: Yifan Gu <yifang@cs.cmu.edu>
 */
#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <string.h>

#include "sha.h"
#include "logger.h"
#include <chunklist.h>
#include "chunk.h"

/**
 * The hashcode in binary
 */
uint8_t hash_bin_buf[SHA1_HASH_SIZE];

#define MAGIC 15441
#define VERSION 1
#define HEADER_SIZE 16
#define PACKET_SIZE 1500 // udp packet size

#define MIN(a, b) ((a) < (b) ? (a) : (b))
/**
 * the max hash number in a packet
 */
#define HASH_NUM_PKT ((MAX_PAYLOAD_SIZE - 4) / SHA1_HASH_SIZE)

/**
 * to be compatible with other implementations
 */
#define EXT_HEADER_SIZE(pkt) (HEADER_SIZE - (pkt)->hdr_len)
#define MAX_PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)
#define PAYLOAD_SIZE 1024
/**
 * Decode / Encode packet from / to buffer
 * @param buf, the raw buf recvfrom peers, it's uint8_t*
 * @param pkt, a pointer to a packet_t struct
 * @return ENCODE_PKT return the size of the packet
 */
#define DECODE_PKT(buf, pkt, size)              \
    memcpy((pkt), (buf), (size));               \
    (pkt)->magic = ntohs((pkt)->magic);         \
    (pkt)->hdr_len = ntohs((pkt)->hdr_len);     \
    (pkt)->pkt_len = ntohs((pkt)->pkt_len);     \
    (pkt)->seq = ntohl((pkt)->seq);             \
    (pkt)->ack = ntohl((pkt)->ack)

#define ENCODE_PKT(buf, pkt, size)                      \
    size_t s = size; /* avoid evaluation after hton */  \
    (pkt)->magic = htons((pkt)->magic);                 \
    (pkt)->hdr_len = htons((pkt)->hdr_len);             \
    (pkt)->pkt_len = htons((pkt)->pkt_len);             \
    (pkt)->seq = htonl((pkt)->seq);                     \
    (pkt)->ack = htonl((pkt)->ack);                     \
    memcpy((buf), (pkt), s);                            
         

/**
 * a couple of macros to make life easier
 */
#define GET_MAGIC(pkt) ((pkt)->magic)
#define SET_MAGIC(pkt, v) ((pkt)->magic = (v))

#define GET_VERSION(pkt) ((pkt)->version)
#define SET_VERSION(pkt, v) ((pkt)->version = (v))

#define GET_TYPE(pkt) ((pkt)->type)
#define SET_TYPE(pkt, v) ((pkt)->type = (v))

#define GET_HDR_LEN(pkt) ((pkt)->hdr_len)
#define SET_HDR_LEN(pkt, v) ((pkt)->hdr_len = (v))

#define GET_PKT_LEN(pkt) ((pkt)->pkt_len)
#define SET_PKT_LEN(pkt, v) ((pkt)->pkt_len = (v))

#define GET_SEQ(pkt) ((pkt)->seq)
#define SET_SEQ(pkt, v) ((pkt)->seq = (v))

#define GET_ACK(pkt) ((pkt)->ack)
#define SET_ACK(pkt, v) ((pkt)->ack = (v))

#define GET_DATA_LEN(pkt) ((pkt)->pkt_len - (pkt)->hdr_len - EXT_HEADER_SIZE((pkt)))
#define GET_DATA(pkt) ((pkt)->payload + EXT_HEADER_SIZE(pkt))

/**
 * Get / Set chunk numbers, EXT_HEADER_SIZE is reserved for other implementations
 * @param pkt, a pointer to a packet_t struct
 * @param cnt, the chunk number to be set
 */
#define GET_CHUNK_CNT(pkt) ((pkt)->payload[0+EXT_HEADER_SIZE(pkt)])
#define SET_CHUNK_CNT(pkt, cnt) ((pkt)->payload[0+EXT_HEADER_SIZE(pkt)] = (cnt))

/**
 * GET / Set hash string
 * "4" is the size of chunk_numbers, EXT_HEADER_SIZE is reserved for other implementations
 * @param pkt, a pointer to a packet_t struct
 * @param hexbuf, the buffer for saving the result HASH string
 * @param hash, the HASH string
 */
#define GET_HASH(pkt, n, hexbuf)                                        \
    binary2hex(((pkt)->payload+EXT_HEADER_SIZE(pkt)                     \
                + (GET_TYPE(pkt) == PACKET_TYPE_GET ? 0 : 4)            \
                + (n)*SHA1_HASH_SIZE), SHA1_HASH_SIZE, (hexbuf))

#define SET_HASH(pkt, n, hash)                                          \
    hex2binary((hash), SHA1_HASH_STR_SIZE, hash_bin_buf);               \
    memcpy((pkt)->payload+EXT_HEADER_SIZE(pkt)                          \
           + (GET_TYPE(pkt) == PACKET_TYPE_GET ? 0 : 4)                 \
           + (n)*SHA1_HASH_SIZE, hash_bin_buf, SHA1_HASH_SIZE)

/**
 * clear the pkt_param_t struct
 */
#define PKT_PARAM_CLEAR(p) (memset(p, 0, sizeof(pkt_param_t)))

/**
 * types of a packet
 */
enum packet_type {
    PACKET_TYPE_WHOHAS = 0,
    PACKET_TYPE_IHAVE  = 1,
    PACKET_TYPE_GET    = 2,
    PACKET_TYPE_DATA   = 3,
    PACKET_TYPE_ACK    = 4,
    PACKET_TYPE_DENIED = 5,
};

/**
 * the packet struct
 */
typedef struct packet_s {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t hdr_len;
    uint16_t pkt_len;
    uint32_t seq;
    uint32_t ack;
    
    uint8_t payload[MAX_PAYLOAD_SIZE]; // either DATA or HASHs
} packet_t;

typedef struct pkt_param_s {
    int socket;
    
    /**
     * the peerlist, start index and count
     */
    //PeerList *p;
    int p_index;
    int p_count;

    /**
     * the chunklist, start index and count
     */
    ChunkList *c;
    int c_index ;
    int c_count;

    /**
     * some packet fields
     */
    uint8_t type;
    uint32_t seq;
    uint32_t ack;

    /**
     * payload and size
     */
    uint8_t *payload;
    size_t payload_size;
    
} pkt_param_t;

/**
 * send packets to peers
 * @param pp, a pointer to pkt_param_t struct
 */
void send_packet(pkt_param_t *pp);

/**
 * check if a packet is valid
 * @param pkt,
 * @return 1 if valid, 0 if not
 */
int valid_packet(packet_t *pkt);

/**
 * a helper for debugging
 */
void print_packet(packet_t *pkt);

#endif /* _PACKET_H_ */
