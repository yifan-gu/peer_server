#ifndef _SEND_HELPER_H
#define _SEND_HELPER_H


void parse_ihavechunks(packet_t *pkt, int);
int parse_download(packet_t *pkt, int);
int send_get(int, int getIndex);

#endif // for #ifndef _SEND_HELPER_H
