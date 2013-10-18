#ifndef _SEND_HELPER_H
#define _SEND_HELPER_H


void parse_ihavechunks(packet_t *pkt, struct sockaddr_in);
int send_get(struct sockaddr_in peer_addr, int getIndex);
int parse_download(packet_t *pkt, struct sockaddr_in peer_addr);

#endif // for #ifndef _SEND_HELPER_H
