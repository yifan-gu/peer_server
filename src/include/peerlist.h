#ifndef _PEERLIST_H
#define _PEERLIST_H

#include <bt_parse.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PEER_NUM BT_MAX_PEERS

typedef struct _Peer {
  struct sockaddr_in addr;
} Peer;

typedef struct _PeerList {
  int count;
  Peer peers[MAX_PEER_NUM];
}PeerList;

int init_peerlist(PeerList *, bt_peer_t *, int);
int addr2Index(PeerList *pl, struct sockaddr_in addr);

#endif // for #ifndef _PEERLIST_H
