#include <peerlist.h>
#include <peer_server.h>
#include <packet.h>
#include <download.h>

extern ChunkList ihavechunks;
extern ChunkList haschunks;
extern ChunkList getchunks;
extern PeerList peerlist;
extern int sock;

void parse_ihavechunks(packet_t *pkt, int p_index) {
    int i, j, k, count;
    char *hexbuf;
    pkt_param_t param;

    count = GET_CHUNK_CNT(pkt);

    for (i = 0, k = 0; i < count; i++) {
        hexbuf = ihavechunks.chunks[k].sha1;
        GET_HASH(pkt, i, hexbuf);
        for (j = 0; j < haschunks.count; j++) {
            if(strcmp(hexbuf, haschunks.chunks[j].sha1) == 0){
                k ++ ;
                break;
            }
        }
    }

    // nothing to send
    if(!k) return;

    ihavechunks.count = k;

    // send IHAVE packet
    PKT_PARAM_CLEAR(&param);
    param.socket = sock;
    param.c = &ihavechunks;
    param.c_count = -1;

    param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = param.p_index == -1? -1: 1;


    param.type = PACKET_TYPE_IHAVE;
    send_packet(&param);
}

int parse_download(packet_t *pkt, int p_index){
    int i, count;
    int fail_flag = 0;

    char *hexbuf;

    Download *dl;
    ChunkLine *cl;
    ll_Node *node;

    dl = & peerlist.peers[p_index].dl;

    if(ll_count(& dl->queue) != 0){
        ll_delete_allnodes(&dl->queue, delete_chunkline);
        init_linkedlist(& dl->queue);
    }

    count = GET_CHUNK_CNT(pkt);
    for (i = 0; i < count; i++) {
        cl = new_chunkline();
        if(! cl){
            fail_flag = 1;
            break;
        }
        node = new_ll_Node(cl);
        if(! node){
            fail_flag = 1;
            break;
        }
        ll_insert_last(&dl->queue, node);

        hexbuf = cl->sha1;
        GET_HASH(pkt, i, hexbuf);
    }

    if(fail_flag) {
        if(ll_count(&dl->queue) != 0){
            ll_delete_allnodes(&dl->queue, delete_chunkline);
        }
        return -1;
    }

    return 0;
}

int send_get(int p_index, int getIndex){
    pkt_param_t param;

    getchunks.chunks[getIndex].state = fetching;

    // send GET packet
    PKT_PARAM_CLEAR(&param);

    param.socket = sock;
    param.c = &getchunks;
    param.c_index = getIndex;
    param.c_count = 1;

    param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = 1;

    param.type = PACKET_TYPE_GET;
    send_packet(&param);

    return 0;
}

/**
 * parse the ack packet from peer[p_index]
 * @param pkt, the packet
 * @param p_index, the peer index
 */
int parse_ack(packet_t *pkt, int p_index) {
    uint32_t ack = GET_ACK(pkt);
    printf("ack: %d\n", ack);

    //tcp_handle_ack(tcp, ack);
    return 0;
}
