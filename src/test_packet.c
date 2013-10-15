#include <stdio.h>
#include <assert.h>

#include "packet.h"
#include "chunk.h"

void check_pkt_header(packet_t *pkt, uint8_t type,
                      uint16_t hdr_len, uint16_t pkt_len, uint32_t seq, uint32_t ack) {
    assert(pkt->magic == MAGIC);
    assert(pkt->version == VERSION);
    assert(pkt->hdr_len == hdr_len);
    assert(pkt->pkt_len == pkt_len);
    assert(pkt->seq == seq);
    assert(pkt->ack == ack);
}

int main(int argc, char *argv[])
{
    packet_t pkt, pkt_clone;
    char buf[PACKET_SIZE];
    char *hash1 = "68938c599df8d02feddde5671a94322b942f0f91";
    char *hash2 = "00c5cbdea37da7925250cc9bbcfcbcc4c28ef915";
    char hash_hex[SHA1_HASH_SIZE*2+1];
    
    printf("Test packeting\n");

    pkt.magic = 15441;
    pkt.version = 1;
    pkt.type = 1; // IHAVE
    pkt.hdr_len = 16;
    pkt.pkt_len = 60;
    
    pkt.seq = 3;
    pkt.ack = 5;
    
    SET_CHUNK_CNT(&pkt, 2);

    SET_HASH(&pkt, 0, hash1);
    
    SET_HASH(&pkt, 1, hash2);

    printf("encoding\n");
    ENCODE_PKT(buf, &pkt, 60);

    printf("decoding\n");
    DECODE_PKT(buf, &pkt_clone, 60);

    printf("checking header\n");
    check_pkt_header(&pkt_clone, PACKET_TYPE_WHOHAS, 16, 60, 3, 5);
    assert(GET_CHUNK_CNT(&pkt_clone) == 2);

    printf("checking chunks\n");
    GET_HASH(&pkt_clone, 0, hash_hex);
    assert(strcmp(hash_hex, hash1) == 0);
    
    GET_HASH(&pkt_clone, 1, hash_hex);
    assert(strcmp(hash_hex, hash2) == 0);
    
    printf("PASS\n");
    
    return 0;
}
