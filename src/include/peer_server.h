#ifndef _PEER_SERVER_H
#define _PEER_SERVER_H

#include <bt_parse.h>
#include <peerlist.h>
#include <chunklist.h>


typedef struct _PeerServer {
    int sock;
    bt_config_t config;

    PeerList peerlist;
    FILE *master_chunk;
    ChunkList haschunks;
    ChunkList getchunks;
    ChunkList ihavechunks;
    int max_conn;
    int dl_num;
    int ul_num;
    int dl_remain;
}PeerServer;


int peer_init(bt_config_t *config);

int find_unfetched_chunk(int);

int addr2Index(struct sockaddr_in addr);
/**
 * look up chunk by the hash
 * @return -1 if not find, otherwise return the index in the chunklist
 */
int hash2Index(ChunkList *clist, const char *hash);
int check_all_timeout();


#endif // for #ifndef _PEER_SERVER_H
