#include <stdio.h>
#include <stdlib.h>

#include "packet.h"
#include "chunk.h"
#include "peer_server.h"

extern PeerList peerlist;
extern ChunkList haschunks;

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in myaddr;
    bt_config_t config;
    char *dummy_data = "hello world";
    
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

    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, -1,
                PACKET_TYPE_WHOHAS, 0, 0,
                NULL, 0);
    
    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, -1,
                PACKET_TYPE_IHAVE, 0, 0,
                NULL, 0);

    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, 1,
                PACKET_TYPE_GET, 0, 0,
                NULL, 0);

    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, -1,
                PACKET_TYPE_DATA, 1, 1,
                (uint8_t *)dummy_data, strlen(dummy_data)+1);

    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, -1,
                PACKET_TYPE_ACK, 1, 1,
                (uint8_t *)dummy_data, strlen(dummy_data)+1);

    send_packet(sock, &peerlist, 0, -1,
                &haschunks, 0, -1,
                PACKET_TYPE_DENIED, 1, 1,
                (uint8_t *)dummy_data, strlen(dummy_data)+1);
    
    return 0;
}
