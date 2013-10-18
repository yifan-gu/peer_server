#include <peerlist.h>
#include <string.h>


int init_peerlist(PeerList *pl, bt_peer_t *peers, int selfid) {
    bt_peer_t *p;
    int i = 0;

    pl->count = 0;
    for (p = peers; p != NULL; p = p->next) {
        if(p->id == selfid)
            continue;
        pl->peers[i].addr = p->addr;
        i ++;
    }
    pl->count = i;

    return 0;
}

int addr2Index(PeerList *pl, struct sockaddr_in addr){
    int i;

    for (i = 0; i < pl->count; i++) {
        if(pl->peers[i].addr.sin_port == addr.sin_port
                && pl->peers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr
          ) {
          return i;
        }
    }
    return -1;
}
