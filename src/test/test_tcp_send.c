#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    fd_set readfds;
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

    int index = 2; // the peer's index in the peerlist
    if (init_tcp_send(&tcp, index, 0) < 0) {
        logger(LOG_ERROR, "tcp init failed");
        return -1;
    }

    int i = 0;
    while (1) {
        int nfds;
        int ret;
        struct sockaddr addr;
        socklen_t socklen;
        struct timeval tv;
        
        char buf[PACKET_SIZE];
        packet_t pkt;
        
        FD_SET(sock, &readfds);
        nfds = select(sock+1, &readfds, NULL, NULL, &tv);

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                ret = recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &socklen);

                if (ret <= 0) {
                    logger(LOG_ERROR, "recvfrom() error");
                    continue;
                }

                DECODE_PKT(buf, &pkt, ret);
                if (PACKET_TYPE_ACK == GET_TYPE(&pkt)) {
                    printf("recv an ack: %d\n", GET_ACK(&pkt));

                    tcp_handle_ack(&tcp, GET_ACK(&pkt));
                }
            }
        }

        printf("%d circle\n", ++i);
        tcp_send_timer(&tcp);
        send_tcp(&tcp);
        dump_tcp_send(&tcp);
        sleep(1);
        
    }

    if (deinit_tcp_send(&tcp) < 0) {
        logger(LOG_ERROR, "deinit failed");
        perror("");
        return -1;
    }
    
    return 0;
}
