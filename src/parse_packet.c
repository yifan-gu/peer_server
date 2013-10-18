#include <parse_packet.h>
#include <peerlist.h>
#include <logger.h>
#include <download.h>
#include <send_helper.h>



int parse_packet(packet_t *pkt, struct sockaddr_in peer_addr) {
    int p_index = addr2Index(peer_addr);
    int getIndex;

    if(p_index < 0)
        return -1;

    switch (GET_TYPE(pkt)) {
    case PACKET_TYPE_WHOHAS:
        parse_ihavechunks(pkt, p_index);
        break;
    case PACKET_TYPE_IHAVE:
        if(parse_download(pkt, p_index) < 0){
            logger(LOG_ERROR, "Failed in parsing IHAVE packet from %s:%d!\n"
              , inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
            return -1;
        }
        getIndex = find_unfetched_chunk(p_index);
        if(getIndex >= 0){
            send_get(p_index, getIndex);
        }
        break;

    case PACKET_TYPE_GET :
        printf("HAHAHA\n");
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

