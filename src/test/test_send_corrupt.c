#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#include "sys/socket.h"
#include "peer_server.h"
#include "peerlist.h"
#include "logger.h"

#define BUFSIZE 50000
//extern PeerList peerlist;

extern PeerServer psvr;

int sock;

void gen_random_data(char *buf, size_t len) {
    int i;
    
    srand(time(NULL));
    for (i = 0; i < len; i++) {
        buf[i] = rand() % 128;
    }

    return;
}

int main(int argc, char *argv[])
{
    char corrupt_date[BUFSIZE];

    int ret, i;
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

    for (i = 0; i < psvr.peerlist.count; i++) {
        gen_random_data(corrupt_date, BUFSIZE);
        ret = sendto(sock, corrupt_date, rand() % BUFSIZE, 0, (struct sockaddr *) & (psvr.peerlist.peers[i].addr),
                     sizeof(psvr.peerlist.peers[i].addr));

        //printf("addr->sin_addr.s_addr == %s\n", inet_ntoa(psvr.peerlist.peers[0].addr.sin_addr));
    
        if (ret < 0) {
            printf("peer.count: %d\n", psvr.peerlist.count);
            logger(LOG_ERROR,"sendto() error\n");
        
            perror("");
        } else {
            printf("sent %d!\n", i);
        }
    }
    
    return 0;
}
