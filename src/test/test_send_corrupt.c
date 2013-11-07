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

void send_udps(int socket, PeerList *p, int p_index, int p_count, uint8_t *buf, size_t len) {
    int i, ret;

    for (i = 0; i < p_count; i++) {
        ret = sendto(socket, buf, len, 0,
                     (struct sockaddr *) & (p->peers[p_index+i].addr),
                     sizeof(p->peers[p_index+i].addr));
        if (ret < 0) {
            logger(LOG_ERROR, "sendto() failed");
        }
    }
}

int main(int argc, char *argv[])
{
    char corrupt_date[BUFSIZE];

    int ret, i;
    packet_t pkt;
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
            printf("sent peer count: %d!\n", i);
        }
    }

    printf("sending malicious packet\n");


    printf("sending pkt with huge pkt len: 10000\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_PKT_LEN(&pkt, 10000);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_HDR_LEN(&pkt, 16);
    SET_CHUNK_CNT(&pkt, 1);

    ENCODE_PKT(corrupt_date, &pkt);

    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending pkt with huge hdr len: 10000\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 10000);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 1500);
    SET_CHUNK_CNT(&pkt, 1);

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);
    
    printf("sending pkt with hdr len > pkt len: 100, 50\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 100);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 50);
    SET_CHUNK_CNT(&pkt, 1);

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending pkt with hdr len = 10\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 10);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 50);
    SET_CHUNK_CNT(&pkt, 1);

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending pkt with chunk cnt == 0\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 16);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 1500);
    SET_CHUNK_CNT(&pkt, 0);
    

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending pkt with chunk cnt == 80\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 16);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 1500);
    SET_CHUNK_CNT(&pkt, 80);
    

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending WHOHASH pkt with chunk cnt == 1 and datalen = 4\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 16);
    SET_TYPE(&pkt, PACKET_TYPE_WHOHAS);
    SET_PKT_LEN(&pkt, 26);
    SET_CHUNK_CNT(&pkt, 1);
    //printf("chunk cnt: %d, MAX_OF: %d, EXT_HDR: %d\n", GET_CHUNK_CNT(&pkt), MAX_OFFSET(&pkt), EXT_HEADER_SIZE(&pkt));
    

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);

    printf("sending GET pkt with chunk cnt == 1 and datalen == 4\n");
    pkt.magic = MAGIC;
    pkt.version = VERSION;
    SET_HDR_LEN(&pkt, 16);
    SET_TYPE(&pkt, PACKET_TYPE_GET);
    SET_PKT_LEN(&pkt, 20);
    SET_CHUNK_CNT(&pkt, 1);
    

    ENCODE_PKT(corrupt_date, &pkt);
    
    send_udps(sock, &psvr.peerlist, 0, psvr.peerlist.count, (uint8_t *)corrupt_date, 1500);
    
    return 0;
}


