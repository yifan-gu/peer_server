#ifndef _PEER_SERVER_H
#define _PEER_SERVER_H

#include <bt_parse.h>
#include <peerlist.h>
#include <chunklist.h>

//#define WINDOW_FILE "problem2-peer.txt"

typedef struct _PeerServer {
    int sock;
    bt_config_t config;
    FILE *w_fp;
    uint32_t start_ts;
    PeerList peerlist;
    FILE *master_chunk;
    ChunkList haschunks;
    ChunkList getchunks;
    ChunkList ihavechunks;
    int max_conn;
    int dl_num;
    int ul_num;
    int dl_remain;
    char dl_filename[BT_FILENAME_LEN];
    char getchunk_file[BT_FILENAME_LEN];
}PeerServer;


int peer_init(bt_config_t *config);

int find_unfetched_chunk(int);
void try_send_get(int p_index);
void refresh_chunk_download();

int addr2Index(struct sockaddr_in addr);
/**
 * look up chunk by the hash
 * @return -1 if not find, otherwise return the index in the chunklist
 */
int hash2Index(ChunkList *clist, const char *hash);
int check_all_timeout();

int write_winsize(int p_index, uint32_t window_size);

#endif // for #ifndef _PEER_SERVER_H
