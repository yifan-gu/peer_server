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
  Peer arr[MAX_PEER_NUM];
}PeerList;

int init_peerlist(PeerList *, bt_peer_t *, int);

#endif // for #ifndef _PEERLIST_H
