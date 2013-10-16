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
    fd_set readfds;
    struct sockaddr_in myaddr;
    bt_config_t config;

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

    printf("listening...\n");
    
    while (1) {
        int nfds;
        int ret;
        struct sockaddr addr;
        socklen_t socklen;
        
        char buf[PACKET_SIZE];
        packet_t pkt;
        
        FD_SET(sock, &readfds);

        nfds = select(sock+1, &readfds, NULL, NULL, NULL);

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                ret = recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &socklen);

                if (ret <= 0) {
                    logger(LOG_ERROR, "recvfrom() error");
                    continue;
                }

                DECODE_PKT(buf, &pkt, ret);

                printf("recv a packet\n");
                print_packet(&pkt);
            }
        }
    }
    
    return 0;
}
