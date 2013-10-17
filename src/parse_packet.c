#include <parse_packet.h>

extern ChunkList ihavechunks;
extern ChunkList haschunks;
extern PeerList peerlist;
extern int sock;

static void parse_ihavechunks(packet_t *pkt, struct sockaddr_in);

int parse_packet(packet_t *pkt, struct sockaddr_in peer_addr) {
    switch (GET_TYPE(pkt)) {
    case PACKET_TYPE_WHOHAS:
        parse_ihavechunks(pkt, peer_addr);
        break;
    case PACKET_TYPE_IHAVE:
        printf("HAHAHA\n");
        break;

    case PACKET_TYPE_GET :
        break;

    case PACKET_TYPE_DATA:
        break;
    case PACKET_TYPE_ACK :
        break;
    default:
        break;
    }

    return 0;
}

static void parse_ihavechunks(packet_t *pkt, struct sockaddr_in peer_addr) {
    int i, j, k, count;
    char *hexbuf;
    pkt_param_t param;

    count = GET_CHUNK_CNT(pkt);

    for (i = 0, k = 0; i < count; i++) {
        hexbuf = ihavechunks.chunks[k].sha1;
        GET_HASH(pkt, i, hexbuf);
        for (j = 0; j < haschunks.count; j++) {
            if(strcmp(hexbuf, haschunks.chunks[j].sha1) == 0){
                k ++ ;
            }
        }
    }

    ihavechunks.count = k;

    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    param.c = &ihavechunks;
    param.c_count = -1;

    param.p = &peerlist;
    param.p_count = -1;
    for (i = 0; i < peerlist.count; i++) {
        if(peerlist.peers[i].addr.sin_port == peer_addr.sin_port
                && peerlist.peers[i].addr.sin_addr.s_addr == peer_addr.sin_addr.s_addr
          ) {
            param.p_index = i;
            param.p_count = 1;
            break;
        }
    }


    param.type = PACKET_TYPE_IHAVE;
    send_packet(&param);
}
