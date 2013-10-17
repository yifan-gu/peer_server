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
