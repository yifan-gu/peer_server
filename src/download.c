#include <string.h>

#include <download.h>
#include <peerlist.h>
#include <peer_server.h>

extern PeerList peerlist;
extern ChunkList getchunks;

int init_download(Download *dl) {
    init_linkedlist(& dl->queue);
    return 0;
}

int find_unfetched_chunk(int p_index) {
    int i;
    Download *dl;
    ll_Node *iter;
    ChunkLine *cl;

    if(p_index < 0) {
        return -1;
    }

    dl = & peerlist.peers[p_index].dl;

    iter = ll_start(&dl->queue);
    while(iter != ll_end(&dl->queue)) {
        cl = (ChunkLine *) iter->item;
        for (i = 0; i < getchunks.count; i++) {
            if(getchunks.chunks[i].state == unfetched
                    && strcmp(getchunks.chunks[i].sha1, cl->sha1) == 0) {
                ll_remove(&dl->queue, iter);
                dl->getIndex = i;
                return i;
            }
        }
        iter = ll_next(iter);
    }

    return -1;
}
