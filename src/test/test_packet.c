#include <stdio.h>
#include <assert.h>

#include "packet.h"
#include "chunk.h"
#include "peer_server.h"

void check_pkt_header(packet_t *pkt, uint8_t type,
                      uint16_t hdr_len, uint16_t pkt_len, uint32_t seq, uint32_t ack, uint8_t cnt) {
    assert(pkt->magic == MAGIC);
    assert(GET_MAGIC(pkt) == MAGIC);

    assert(pkt->version == VERSION);
    assert(GET_VERSION(pkt) == VERSION);

    assert(pkt->type == type);
    assert(GET_TYPE(pkt) == type);

    assert(pkt->hdr_len == hdr_len);
    assert(GET_HDR_LEN(pkt) == hdr_len);

    assert(pkt->pkt_len == pkt_len);
    assert(GET_PKT_LEN(pkt) == pkt_len);

    assert(pkt->seq == seq);
    assert(GET_SEQ(pkt) == seq);

    assert(pkt->ack == ack);
    assert(GET_ACK(pkt) == ack);

    if (pkt->type == PACKET_TYPE_IHAVE
        || pkt->type == PACKET_TYPE_WHOHAS
        || pkt->type == PACKET_TYPE_GET) {
        
        assert(GET_CHUNK_CNT(pkt) == cnt);
    }
}

int main(int argc, char *argv[])
{
    packet_t pkt, pkt_clone;
    char buf[PACKET_SIZE*10];
    char *hash1 = "68938c599df8d02feddde5671a94322b942f0f91";
    char *hash2 = "00c5cbdea37da7925250cc9bbcfcbcc4c28ef915";
    char *data = "this is a dummy data";
    char hash_hex[SHA1_HASH_SIZE*2+1];
    char *filename = "tmp/Big.chunks";

    ChunkList cl;
    int i;
    
    printf("Test packeting\n");

    SET_MAGIC(&pkt, 15441);
    SET_VERSION(&pkt, 1);
    SET_TYPE(&pkt, 1); // IHAVE
    SET_HDR_LEN(&pkt, 16);
    SET_PKT_LEN(&pkt, 60);

    SET_SEQ(&pkt, 3);
    SET_ACK(&pkt, 5);

    SET_CHUNK_CNT(&pkt, 2);

    SET_HASH(&pkt, 0, hash1);

    SET_HASH(&pkt, 1, hash2);

    printf("encoding\n");
    ENCODE_PKT(buf, &pkt, 60);

    print_packet(&pkt);
    
    printf("decoding\n");
    DECODE_PKT(buf, &pkt_clone, 60);

    print_packet(&pkt_clone);
    
    printf("checking header\n");
    check_pkt_header(&pkt_clone, PACKET_TYPE_IHAVE, 16, 60, 3, 5, 2);
    assert(GET_CHUNK_CNT(&pkt_clone) == 2);

    printf("checking chunks\n");
    GET_HASH(&pkt_clone, 0, hash_hex);
    assert(strcmp(hash_hex, hash1) == 0);

    GET_HASH(&pkt_clone, 1, hash_hex);
    assert(strcmp(hash_hex, hash2) == 0);

    printf("testing send_message, will write 3 packets containning 150 hashes...\n");
    assert(0 == parse_chunk(&cl, filename));
    
    send_message(0, NULL, 0, 0, &cl, 0, -1, PACKET_TYPE_WHOHAS, 0, 0, NULL, 0);
    
    printf("testing send_message(data)\n");
    send_message(0, NULL, 0, 0, &cl, 0, -1, PACKET_TYPE_DATA, 1, 1, data, strlen(data)+1);

    printf("PASS\n");

    return 0;
}


void test_message(uint8_t *buf, int i, ChunkList *cl) {

    char hash_hex[SHA1_HASH_STR_SIZE+1];
    packet_t pkt_clone;
    char *data = "this is a dummy data";
    
    DECODE_PKT(buf, &pkt_clone, 1500);

    print_packet(&pkt_clone);

    /* check data packet */
    if (GET_TYPE(&pkt_clone) == PACKET_TYPE_DATA) {

        check_pkt_header(&pkt_clone, PACKET_TYPE_DATA, 16, strlen(data)+1+HEADER_SIZE, 1, 0, 0);

        assert(0 == strcmp(data, GET_PAYLOAD(&pkt_clone)));

        return;
    }
    
    check_pkt_header(&pkt_clone, PACKET_TYPE_WHOHAS, 16, i == 2?60:PACKET_SIZE, 0, 0, i == 2?2:HASH_NUM_PKT);

    int j = 0;
        
    for (j = 0; j < (i == 2 ? 2 : HASH_NUM_PKT); j++) {
        GET_HASH(&pkt_clone, j, hash_hex);
        assert(0 == strcmp(hash_hex, cl->chunks[j+i*HASH_NUM_PKT].sha1));
    }
}
