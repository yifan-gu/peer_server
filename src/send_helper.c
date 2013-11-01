#include <peerlist.h>
#include <peer_server.h>
#include <packet.h>
#include <download.h>

extern ChunkList ihavechunks;
extern ChunkList haschunks;
extern ChunkList getchunks;
extern PeerList peerlist;
extern int sock;

// send whohas

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

    //param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = param.p_index == -1? -1: 1;


    param.type = PACKET_TYPE_IHAVE;
    send_packet(&param);
}

int parse_download(packet_t *pkt, int p_index){
    int i, count;
    int fail_flag = 0;

    char *hexbuf;

    download_t *dl;
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
            init_linkedlist(& dl->queue);
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

    //param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = 1;

    param.type = PACKET_TYPE_GET;
    send_packet(&param);

    return 0;
}

/**
 * start download
 */
int start_download(Download *dl, int p_index, int get_index, const char *filename) {
    return dl_init(dl, p_index, get_index, filename);
}

/**
 * update download info when get a DATA
 */
int update_download(Download *dl, packet_t *pkt) {
    return dl_recv(dl, pkt);
}

/**
 * finish download
 */
int finish_download(Download *dl) {
    return dl_finish(dl);
}

/**
 * kill download
 */
int kill_download(Download *dl) {
    return 0;
}

/**
 * check if download is OK
 * @return 1 if finished, 0 if not, -1 if hash is not correct
 */
int is_download_finished(Download *ul) {
    if (!ul->finished) {
        return 0;
    }

    if (dl_check_hash() == 0) {
        return 1;
    } else {
        return -1;
    }
}

/**
 * start the upload
 */
int start_upload(Upload *ul, int p_index, int has_index) {
    return ul_init(ul, p_index, has_index);
}

/**
 * update upload info when get an ACK
 */
int update_upload(Upload *ul, packet_t *pkt) {
    ul_handle_ack(ul, GET_ACK(pkt));
    return 0;
}

/**
 * check if upload is finished
 */
int is_upload_finished(Upload *ul) {
    return ul->finished;
}

/**
 * finish upload
 */
int finish_upload(Upload *ul) {
    return ul_deinit(ul);
}


