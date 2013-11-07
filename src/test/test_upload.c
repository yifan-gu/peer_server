#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "upload.h"
#include "packet.h"
#include "chunk.h"
#include "peer_server.h"
#include "peerlist.h"
#include "spiffy.h"
#include "logger.h"

extern PeerList peerlist;
extern ChunkList haschunks;
extern FILE *log_fp;
extern PeerServer psvr;

int sock;

int main(int argc, char *argv[])
{
    fd_set readfds;
    struct sockaddr_in myaddr;
    //char *dummy_data = "hello world";
    upload_t ul;

    init_log("upload.log");
    
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
    psvr.sock = sock;


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
    
    int index = 1; // the peer's index in the peerlist
    if (ul_init(&ul, index, 0) < 0) {
        logger(LOG_ERROR, "ul init failed");
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
        tv.tv_usec = 5000*100;
        nfds = select(sock+1, &readfds, NULL, NULL, &tv);

        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
                ret = spiffy_recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &socklen);

                if (ret <= 0) {
                    logger(LOG_ERROR, "recvfrom() error");
                    continue;
                }

                DECODE_PKT(buf, &pkt, ret);
                //print_packet(&pkt);
                if (PACKET_TYPE_ACK == GET_TYPE(&pkt)) {
                    logger(LOG_DEBUG, "recv an ack: %d\n", GET_ACK(&pkt));
                    printf("recv an ack: %d\n", GET_ACK(&pkt));
                    printf("%d circle\n", ++i);
                    ul_dump(&ul, log_fp);

                    ul_handle_ack(&ul, GET_ACK(&pkt));
                }
            }
        }

        //printf("%d circle\n", ++i);
        ul_check_timeout(&ul);
        ul_send(&ul);
        
        //sleep(1);
        //gettimeofday(&ts, NULL);
        //srandom(ts.tv_sec);

        //uint32_t sleeptime = 500 + random() % 500; // sleep 500 - 1500 ms
        //printf("sleep for: %dms\n", sleeptime);
        //usleep(sleeptime * 1000);

        if (ul.finished) {
            printf("finished!\n");
            break;
        }
    }

    if (ul_deinit(&ul) < 0) {
        logger(LOG_ERROR, "deinit failed");
        perror("");
        return -1;
    }

    deinit_log();
    
    return 0;
}
