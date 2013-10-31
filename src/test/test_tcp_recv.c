#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "packet.h"
#include "tcp_recv.h"
#include "chunk.h"
#include "peer_server.h"
#include "spiffy.h"

extern PeerList peerlist;
extern ChunkList haschunks;
int sock;

void handle_user_input(char *line, void *cbdata) {
    uint32_t ack;
    pkt_param_t param;
    
    if (sscanf(line, "%d", &ack)) {
        PKT_PARAM_CLEAR(&param);
        param.socket = sock;
        param.p = &peerlist;
        param.p_count = -1;
        param.c = &haschunks;
        param.c_count = 0;
        param.seq = 0;
        param.ack = ack;
        param.type = PACKET_TYPE_ACK;
        send_packet(&param);
    }
}

int main(int argc, char *argv[])
{
    fd_set readfds;
    struct sockaddr_in myaddr;
    bt_config_t config;
    tcp_recv_t tcp;

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
    
    printf("listening...\n");
    
    int index = 1; // the peer's index in the peerlist
    if (init_tcp_recv(&tcp, index, 0, "test.log") < 0) {
        logger(LOG_ERROR, "tcp init failed");
        return -1;
    }

    int i = 0;
    while (1) {
        int nfds;
        int ret;
        struct sockaddr addr;
        socklen_t socklen;
        struct timeval tv;//, ts;
        
        char buf[PACKET_SIZE];
        packet_t pkt;
        
        FD_SET(sock, &readfds);
        
        nfds = select(sock+1, &readfds, NULL, NULL, &tv);
        
        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                ret = spiffy_recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &socklen);

                if (ret <= 0) {
                    logger(LOG_ERROR, "recvfrom() error");
                    continue;
                }

                DECODE_PKT(buf, &pkt, ret);

                //printf("recv a packet\n");
                //print_packet(&pkt);
                logger(LOG_DEBUG, "recv seq: %d\n", GET_SEQ(&pkt));
                printf("%d circle\n", ++i);
                dump_tcp_recv(&tcp);

                recv_tcp(&tcp, &pkt);
            }

        }

        
        tcp_recv_timer(&tcp);
        
        //sleep(1);
        //gettimeofday(&ts, NULL);
        //srandom(ts.tv_sec);

        //uint32_t sleeptime = 500 + random() % 1000; // sleep 500 - 1500 ms
        //printf("sleep for: %dms\n", sleeptime);
        //usleep(sleeptime * 1000);

        if (tcp.finished) {
            printf("finished\n");
            break;
        }
    }

    save_buffer(&tcp);
    
    return 0;
}
