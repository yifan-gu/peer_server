#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <string.h>

#include "sha.h"

/**
 * The hashcode in binary
 */
char hash_bin_buf[SHA1_HASH_SIZE];

#define MAGIC 15441
#define VERSION 1
#define HEADER_SIZE 16
#define PACKET_SIZE 1500 // udp packet size
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)

/**
 * Decode / Encode packet from / to buffer
 * @param buf, the raw buf recvfrom peers, it's uint8_t*
 * @param pkt, a pointer to a packet_t struct
 */
#define DECODE_PKT(buf, pkt, size)                                      \
    memcpy((pkt), (buf), (size));                                       \
    pkt->magic = ntohs(pkt->magic);                                     \
    pkt->hdr_len = ntohs(pkt->hdr_len);                                 \
    pkt->pkt_len = ntohs(pkt->pkt_len);                                 \
    pkt->seq = ntohl(pkt->seq);                                         \
    pkt->ack = ntohl(pkt->ack)

#define ENCODE_PKT(buf, pkt, size)                                      \
    pkt->magic = htons(pkt->magic);                                     \
    pkt->hdr_len = htons(pkt->hdr_len);                                 \
    pkt->pkt_len = htons(pkt->pkt_len);                                 \
    pkt->seq = htonl(pkt->seq);                                         \
    pkt->ack = htonl(pkt->ack);                                         \
    memcpy((buf), (pkt), (size))

/**
 * Get / Set chunk numbers
 * @param pkt, a pointer to a packet_t struct
 * @param cnt, the chunk number to be set
 */
#define GET_CHUNK_CNT(pkt) ((pkt)->payload[0])
#define SET_CHUNK_CNT(pkt, cnt) ((pkt)->payload[0] = (cnt))

/**
 * GET / Set hash string
 * "4" is the size of chunk_numbers
 * @param pkt, a pointer to a packet_t struct
 * @param hexbuf, the buffer for saving the result HASH string
 * @param hash, the HASH string
 */
#define GET_HASH(pkt, n, hexbuf) binary2hex(((pkt)->payload+4+(n)*SHA1_HASH_SIZE), SHA1_HASH_SIZE, (hexbuf))

#define SET_HASH(pkt, n, hash)                                          \
    hex2binary((hash), SHA1_HASH_SIZE*2, hash_bin_buf);                 \
    memcpy((pkt)->payload+4+(n)*SHA1_HASH_SIZE, hash_bin_buf, SHA1_HASH_SIZE)

/**
 * types of a packet
 */
enum packet_type {
    PACKET_TYPE_WHOHAS = 0,
    PACKET_TYPE_IHAVE = 1,
    PACKET_TYPE_DATA = 2,
    PACKET_TYPE_GET = 3,
    PACKET_TYPE_ACK = 4,
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
    
    uint8_t payload[PAYLOAD_SIZE]; // either DATA or HASHs
} packet_t;

#endif
