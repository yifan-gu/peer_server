#ifndef _PARSE_PACKET_H
#define _PARSE_PACKET_H

#include <packet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int parse_packet(packet_t *, struct sockaddr_in);

#endif // for #ifndef _PARSE_PACKET_H
