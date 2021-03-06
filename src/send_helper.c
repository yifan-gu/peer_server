/*
@brief
  This module provides helper functions which encapsulates the logic of sending and handling different types of packets.
 */

#include <peerlist.h>
#include <peer_server.h>
#include <chunklist.h>
#include <packet.h>
#include <download.h>
#include <sha.h>

extern PeerServer psvr;

// send whohas
void send_whohas() {
    pkt_param_t param;

    PKT_PARAM_CLEAR(&param);
    param.socket = psvr.sock;
    param.p_count = -1;
    param.c = &psvr.getchunks;
    param.c_count = -1;

    param.type = PACKET_TYPE_WHOHAS;
    printf("Finding chunks...\n");
    send_packet(&param);
}

void send_ihave(packet_t *pkt, int p_index) {
    int i, j, k, count;
    char *hexbuf;
    pkt_param_t param;

    count = GET_CHUNK_CNT(pkt);

    for (i = 0, k = 0; i < count; i++) {
        hexbuf = psvr.ihavechunks.chunks[k].sha1;
        GET_HASH(pkt, i, hexbuf);
        for (j = 0; j < psvr.haschunks.count; j++) {
            if(strcmp(hexbuf, psvr.haschunks.chunks[j].sha1) == 0) {
                k ++ ;
                break;
            }
        }
    }

    // nothing to send
    if(!k) return;
    
    psvr.ihavechunks.count = k;

    // send IHAVE packet
    PKT_PARAM_CLEAR(&param);
    param.socket = psvr.sock;
    param.c = &psvr.ihavechunks;
    param.c_count = -1;

    //param.p = &peerlist;
    param.p_index = p_index;
    param.p_count = param.p_index == -1? -1: 1;


    param.type = PACKET_TYPE_IHAVE;
    printf("Replying IHAVE...\n");
    send_packet(&param);
}

// This will parse the ihave packet chunks and add them to corresponding peer object.
// @return
//  if succeeded, 0;
//  if failed, -1;
int parse_download(packet_t *pkt, int p_index) {
    int i, count;
    int fail_flag = 0;

    char *hexbuf;

    Linlist *dq;
    ChunkLine *cl;
    ll_Node *node;

    dq = & psvr.peerlist.peers[p_index].hasqueue;

    if(ll_count(dq) != 0) {
        ll_delete_allnodes(dq, delete_chunkline);
        init_linkedlist(dq);
    }

    count = GET_CHUNK_CNT(pkt);
    for (i = 0; i < count; i++) {
        cl = new_chunkline();
        if(! cl) {
            fail_flag = 1;
            break;
        }
        node = new_ll_Node(cl);
        if(! node) {
            fail_flag = 1;
            break;
        }
        ll_insert_last(dq, node);

        hexbuf = cl->sha1;
        GET_HASH(pkt, i, hexbuf);
    }

    if(fail_flag) {
        if(ll_count(dq) != 0) {
            ll_delete_allnodes(dq, delete_chunkline);
            init_linkedlist(dq);
        }
        return -1;
    }

    return 0;
}

int send_get(int p_index, int getIndex) {
    pkt_param_t param;

    psvr.getchunks.chunks[getIndex].state = fetching;

    // send GET packet
    PKT_PARAM_CLEAR(&param);

    param.socket = psvr.sock;
    param.c = &psvr.getchunks;
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
int start_download(Download *dl, int p_index, int get_index, const char filename[BT_CHUNK_SIZE]) {
    if( dl_init(dl, p_index, get_index, filename) < 0){
        return -1;
    }
    psvr.dl_num ++;
    return 0;
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
    /*return dl_finish(dl);*/
    return 0;
}

/**
 * kill download
 */
int kill_download(Download *dl) {
    psvr.getchunks.chunks[dl->get_index].state = unfetched;
    psvr.dl_num --;
    return 0;
}

/**
 * check if download is OK
 * @return 1 if finished, 0 if not.
 */
int is_download_finished(Download *dl) {
    return (dl->finished)? 1: 0 ;
}

// After finishing download, this functions will compute the SHA1 hash of entire chunk
// and compare it against the expected hash value.
// @return
//  1 on success, 0 on failure
int check_hash_succeed(Download *dl) {
    uint8_t hash[SHA1_HASH_SIZE];
    char hash_str[SHA1_HASH_SIZE * 2 + 1];

    SHA1Context sc;
    SHA1Init(&sc);
    SHA1Update(&sc, dl->buffer, BT_CHUNK_SIZE);
    SHA1Final(&sc, hash);

    binary2hex(hash, SHA1_HASH_SIZE, hash_str);

    if( strncmp(hash_str,
                psvr.getchunks.chunks[dl->get_index].sha1,
                SHA1_HASH_STR_SIZE) == 0) {
        return 1;
    }
    else {
        logger(LOG_WARN, "Check hash not correct:\nexpected: %s\nreceived: %s\n",
               psvr.getchunks.chunks[dl->get_index].sha1, hash_str)
        return 0;
    }
}

int write_to_file(Download *dl) {
    return dl_save_buffer(dl);
}

/**
 * start the upload
 */
int start_upload(Upload *ul, int p_index, int has_index) {
    if (ul_init(ul, p_index, has_index) < 0) {
        logger(LOG_INFO, "upload init failed");
        return -1;
    }

    if( ul_send(ul) < 0 )
        return -1;
    psvr.ul_num ++;
    return 0;
}

/**
 * update upload info when get an ACK
 */
int update_upload(Upload *ul, packet_t *pkt) {
    ul_handle_ack(ul, GET_ACK(pkt));
    return ul_send(ul);
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
    psvr.ul_num --;
    return ul_deinit(ul);
}

/**
 * print download progress
 */
void print_download_progress(Peer* p) {
    int c_id;
    int p_id;
    int progress;
    
    if (0 == p->dl.data_length) {
        return;
    }
    
    c_id = psvr.getchunks.chunks[p->dl.get_index].id;
    p_id = p->id;
    progress = p->dl.next_pkt_expected * 100 / (BT_CHUNK_SIZE / p->dl.data_length);
    
    printf("Downloading chunk[%d] from peer[%d]: %d%% completed\r", c_id, p_id, MIN(100, progress));
    if (progress >= 100) {
        printf("\n");
    }

    return;
}
