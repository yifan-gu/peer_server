#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcp.h"
#include "packet.h"
#include "chunk.h"
#include "peer_server.h"

extern PeerList peerlist;
extern ChunkList haschunks;

bt_config_t config;
int sock;

int main(int argc, char *argv[])
{
    struct sockaddr_in myaddr;
    //char *dummy_data = "hello world";
    tcp_send_t tcp;
    
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

    int index = 2;
    if (init_tcp_send(&tcp, index, 0) < 0) {
        logger(LOG_ERROR, "tcp init failed");
        return -1;
    }

    while (tcp.last_pkt_acked < 1) {
        printf("sending...\n");
        if (send_tcp(&tcp) < 0) {
            logger(LOG_ERROR, "send tcp failed");
            return -1;
        }

        tcp.last_pkt_acked ++;
    }

    if (deinit_tcp_send(&tcp) < 0) {
        logger(LOG_ERROR, "deinit failed");
        perror("");
        return -1;
    }
    
    return 0;
}
