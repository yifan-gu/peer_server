#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packet.h"
#include "chunk.h"
#include "peer_server.h"
#include "peerlist.h"
#include "input_buffer.h"
#include "spiffy.h"

//extern PeerList peerlist;
//extern ChunkList haschunks;
extern PeerServer psvr;

int sock;

void handle_user_input(char *line, void *cbdata) {
    uint32_t seq;
    pkt_param_t param;
    char data[900];
    
    
    if (sscanf(line, "%d", &seq)) {
        sprintf(data, "data packet[%d]", seq);
        PKT_PARAM_CLEAR(&param);
        param.socket = sock;
        //param.p = &peerlist;
        param.p_count = -1;
        param.c = &psvr.haschunks;
        param.c_count = 0;
        param.seq = seq;
        param.ack = 0;
        param.type = PACKET_TYPE_DATA;
        param.payload = (uint8_t *)data;
        /*if (12 == seq) {
            param.payload_size = 340;
            } else */
            param.payload_size = sizeof(data);
        send_packet(&param);
    }
}

int main(int argc, char *argv[])
{
    fd_set readfds;
    struct sockaddr_in myaddr;
    struct user_iobuf *userbuf;
    bt_init(&psvr.config, argc, argv);
    init_log(NULL);


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
    
    if ((userbuf = create_userbuf()) == NULL) {
        perror("peer_run could not allocate userbuf");
        exit(-1);
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
        
        FD_SET(STDIN_FILENO, &readfds);
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
                //print_packet(&pkt);
                printf("packet ack: %d\n", GET_ACK(&pkt));
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                process_user_input(STDIN_FILENO, userbuf, handle_user_input,
                                   "Currently unused");
            }
        }
    }
    
    return 0;
}
