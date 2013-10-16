#include <parse_packet.h>

extern ChunkList ihavechunks;
extern PeerList peerlist;
extern int sock;

static void parse_ihavechunks(packet_t *pkt);

int parse_packet(packet_t *pkt){
  switch (GET_TYPE(pkt)) {
  case PACKET_TYPE_WHOHAS:
      parse_ihavechunks(pkt);
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

static void parse_ihavechunks(packet_t *pkt){
    int i, count;
    char *hexbuf;
    pkt_param_t param;

    count = GET_CHUNK_CNT(pkt);

    for (i = 0; i < count; i++) {
        hexbuf = ihavechunks.chunks[i].sha1;
        GET_HASH(pkt, i, hexbuf);
    }

    ihavechunks.count = count;

    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    param.p = &peerlist;
    param.p_count = -1;
    param.c = &ihavechunks;
    param.c_count = -1;

    param.type = PACKET_TYPE_IHAVE;
    send_packet(&param);
}
