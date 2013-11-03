#include <stdio.h>
#include <stdlib.h>

#include "packet.h"
#include "chunk.h"
#include "peer_server.h"
#include "peerlist.h"
#include "spiffy.h"

//extern PeerList peerlist;
//extern ChunkList haschunks;
extern PeerServer psvr;
int sock;

int main(int argc, char *argv[])
{
    struct sockaddr_in myaddr;
    bt_config_t config;
    char *dummy_data = "hello world";
    pkt_param_t param;
    
    bt_init(&config, argc, argv);

#ifdef TESTING
    config.identity = 1; // your group number here
    strcpy(config.chunk_file, "chunkfile");
    strcpy(config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&config);

    if(peer_init(&config) < 0) {
        logger(LOG_ERROR, "Peer init failed!");
        exit(0);
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(config.myport);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }
    
    /* init spiffy */
    spiffy_init(config.identity, (struct sockaddr *) &myaddr, sizeof(myaddr));

    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    //param.p = &peerlist;
    param.p_count = -1;
    param.c = &psvr.haschunks;
    param.c_count = -1;
    param.seq = 1; // should have no effect on nondata packets
    param.ack = 1;
    param.payload = (uint8_t *)dummy_data; // should have no effect on nondata packets
    param.payload_size = strlen(dummy_data)+1;
    
    param.type = PACKET_TYPE_WHOHAS;
    send_packet(&param);

    param.type = PACKET_TYPE_IHAVE;
    send_packet(&param);

    param.type = PACKET_TYPE_GET;
    send_packet(&param);

    param.type = PACKET_TYPE_DATA;
    
    send_packet(&param);                

    param.type = PACKET_TYPE_ACK;
    send_packet(&param);

    param.type = PACKET_TYPE_DENIED;
    send_packet(&param);
    
    return 0;
}
