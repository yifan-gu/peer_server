#include <stdio.h>
#include <stdlib.h>

#include "packet.h"
#include "chunk.h"
#include "peer_server.h"
#include "peerlist.h"
#include "spiffy.h"

extern PeerList peerlist;
extern ChunkList haschunks;
extern PeerServer psvr;

int sock;

int main(int argc, char *argv[])
{
    fd_set readfds;
    struct sockaddr_in myaddr;
    init_log(NULL);
    bt_init(&psvr.config, argc, argv);

#ifdef TESTING
    psvr.config.identity = 1; // your group number here
    strcpy(psvr.config.chunk_file, "chunkfile");
    strcpy(psvr.config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&psvr.config);

    if(peer_init(&psvr.config) < 0) {
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
    myaddr.sin_port = htons(psvr.config.myport);

    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }

    /* init spiffy */
    spiffy_init(psvr.config.identity, (struct sockaddr *) &myaddr, sizeof(myaddr));
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
                ret = spiffy_recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &socklen);

                if (ret <= 0) {
                    logger(LOG_ERROR, "recvfrom() error");
                    continue;
                }
                
                DECODE_PKT(buf, &pkt, ret);
                printf("recv a packet\n");
                
                if (valid_packet(&pkt)) {
                    printf("validation ok\n");
                    print_packet(&pkt);
                }
            }
        }
    }
    
    return 0;
}
