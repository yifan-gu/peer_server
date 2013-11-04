/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <spiffy.h>
#include <bt_parse.h>
#include <input_buffer.h>

#include <logger.h>
#include <peer_server.h>
#include "peerlist.h"
#include <packet.h>
#include <parse_packet.h>
#include <send_helper.h>

extern PeerServer psvr;

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {

    init_log(NULL);

    bt_init(&psvr.config, argc, argv);

    DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
    psvr.config.identity = 1; // your group number here
    strcpy(psvr.config.chunk_file, "chunkfile");
    strcpy(psvr.config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&psvr.config);

#ifdef DEBUG
    if (debug & DEBUG_INIT) {
        bt_dump_config(&psvr.config);
    }
#endif

    if(peer_init(&psvr.config) < 0) {
        logger(LOG_ERROR, "Peer init failed!");
        exit(0);
    }

    peer_run(&psvr.config);
    deinit_log();

    return 0;
}


void process_inbound_udp(int sock) {
#define BUFLEN 1500
    struct sockaddr_in from;
    socklen_t fromlen;
    char buf[BUFLEN];
    packet_t pkt;
    int ret;

    fromlen = sizeof(from);
    ret = spiffy_recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from, &fromlen);

    printf("PROCESS_INBOUND_UDP SKELETON -- replace!\n"
           "Incoming message from %s:%d\n\n\n",
           inet_ntoa(from.sin_addr),
           ntohs(from.sin_port));


    DECODE_PKT(buf, &pkt, ret);
    print_packet(&pkt);

    if (!valid_packet(&pkt)) {
        logger(LOG_INFO, "Invalid packet received!");
        return;
    }
    parse_packet(&pkt, from);
}


void process_get(char *chunkfile, char *outputfile) {

    printf("PROCESS GET SKELETON CODE CALLED.  Fill me in!  (%s, %s)\n",
           chunkfile, outputfile);

    if(strlen(outputfile) >= BT_FILENAME_LEN){
        printf("Destination filename is too long!\n");
        return;
    }

    strcpy(psvr.dl_filename, outputfile);

    if ( parse_chunk(&psvr.getchunks, chunkfile) < 0 ){
        logger(LOG_WARN, "Can't parse chunk file: %s", chunkfile);
        return;
    }
    psvr.dl_remain = psvr.getchunks.count;

    // send whohas
    send_whohas();
    
    psvr.last_start = get_timestamp_now();
}

void handle_user_input(char *line, void *cbdata) {
    char chunkf[128], outf[128];

    bzero(chunkf, sizeof(chunkf));
    bzero(outf, sizeof(outf));

    if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
        if (strlen(outf) > 0) {
            process_get(chunkf, outf);
        }
    }
}

void peer_run(bt_config_t *config) {
    struct sockaddr_in myaddr;
    fd_set readfds;
    struct user_iobuf *userbuf;

    if ((userbuf = create_userbuf()) == NULL) {
        perror("peer_run could not allocate userbuf");
        exit(-1);
    }

    if ((psvr.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }

    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // For testing
    myaddr.sin_port = htons(config->myport);

    if (bind(psvr.sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }

    spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

    while (1) {
        int nfds;
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(psvr.sock, &readfds);

        // make sure we check timeout at least every 100 ms
        struct timeval tv = {0, 100};
        nfds = select(psvr.sock+1, &readfds, NULL, NULL, &tv);

        if (nfds > 0) {
            if (FD_ISSET(psvr.sock, &readfds)) {
                process_inbound_udp(psvr.sock);
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                process_user_input(STDIN_FILENO, userbuf, handle_user_input,
                                   "Currently unused");
            }
        }

        if ((psvr.dl_remain > 0)
            && (get_timestamp_now() - psvr.last_start) > DEFAULT_TIMEOUT) {
            check_all_timeout();
        }
    }
}
