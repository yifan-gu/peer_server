#include <peerlist.h>
#include <string.h>
#include "download.h"

int init_peer(Peer *pr){
    pr->is_alive = 0;
    pr->is_downloading = 0;
    pr->is_uploading = 0;
    //init_download(& pr->dl);
    return 0;
}

void die(Peer *pr){
    pr->is_alive = 0;
}

int init_peerlist(PeerList *pl, bt_peer_t *peers, int selfid) {
    bt_peer_t *p;
    int i = 0;

    pl->count = 0;
    for (p = peers; p != NULL; p = p->next) {
        if(p->id == selfid)
            continue;

        // init peer
        pl->peers[i].addr = p->addr;
        init_peer(&pl->peers[i]);
        i ++;
    }
    pl->count = i;

    return 0;
}

