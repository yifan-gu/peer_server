#include <peer_server.h>
#include <malloc.h>

#include <logger.h>
#include "send_helper.h"

#define BUFFER 256

PeerServer psvr;

char *readString(FILE *fp)
{
    char *str = malloc(sizeof(char) * BUFFER), *err;
    int pos;
    for(pos = 0; (str[pos] = fgetc(fp)) != '\n'; pos++)
    {
        if(pos % BUFFER == BUFFER - 1)
        {
            if((err = realloc(str, sizeof(char) * (BUFFER + pos + 1))) == NULL) {
                free(str);
                return NULL;
            }
        }
    }
    str[pos] = '\0';
    return str;
}

int peer_init(bt_config_t *config) {
    FILE *tmp_fp;
    char *file_str;

    psvr.start_ts = get_timestamp_now();
    if (0 == psvr.start_ts) {
        logger(LOG_ERROR, "get_timestamp_now() error");
        return -1;
    }

    init_peerlist((PeerList *) & psvr.peerlist, config->peers, config->identity);

    // master chunk_file
    tmp_fp = fopen(config->chunk_file, "r");
    if(tmp_fp == NULL) {
        logger(LOG_ERROR, "can't open master chunk_file: %s", config->chunk_file);
        return -1;
    }
    {
        fscanf(tmp_fp, "File:%*c");
        file_str = readString(tmp_fp);
    }
    fclose(tmp_fp);

    if(file_str == NULL) {
        logger(LOG_ERROR, "can't parse master chunk_file");
        return -1;
    }

    psvr.master_chunk = fopen(file_str, "r");
    if(psvr.master_chunk == NULL) {
        logger(LOG_ERROR, "can't open (%s) for master_chunk reading", file_str);
        return -1;
    }
    free(file_str);

    psvr.w_fp = fopen(psvr.config.output_file, "w+");
    if (NULL == psvr.w_fp) {
        logger(LOG_ERROR, "can't open (%s) for window_size file", psvr.config.output_file);
        return -1;
    }

    // has_chunk_file
    if(parse_chunk(&psvr.haschunks, config->has_chunk_file) < 0 ) {
        return -1;
    }
    psvr.getchunks.count = 0;

    // max_conn;
    psvr.max_conn = config->max_conn;
    psvr.dl_num = 0;
    psvr.dl_remain = 0;
    psvr.ul_num = 0;
    return 0;
}

int find_unfetched_chunk(int p_index) {
    int i;
    Linlist *dq;
    ll_Node *iter;
    ChunkLine *cl;

    if(p_index < 0) {
        return -1;
    }

    dq = & psvr.peerlist.peers[p_index].hasqueue;

    iter = ll_start(dq);
    while(iter != ll_end(dq)) {
        cl = (ChunkLine *) iter->item;
        for (i = 0; i < psvr.getchunks.count; i++) {
            if(psvr.getchunks.chunks[i].state == unfetched
                    && strcmp(psvr.getchunks.chunks[i].sha1, cl->sha1) == 0) {
                ll_remove(dq, iter);
                psvr.peerlist.peers[p_index].dl.get_index = i;
                return i;
            }
        }
        iter = ll_next(iter);
    }

    return -1;
}

void try_send_get(int p_index) {
    int getIndex;
    Peer *peer_p;

    getIndex = find_unfetched_chunk(p_index);
    if(getIndex >= 0) {
        peer_p = &psvr.peerlist.peers[p_index];
        send_get(p_index, getIndex);
        // start peer download
        psvr.dl_num ++;
        peer_p->is_downloading = 1;
        start_download(& peer_p->dl, p_index, getIndex, psvr.dl_filename);
    }
}

#define NOT_GOOD_FOR_DOWNLOAD(p) ( ! (p)->is_alive || (p)->is_downloading )

// find another one to download
void refresh_chunk_download() {
    int i;
// iterate each peer
//   if find_unfetched_chunk(peer) succeed
//     send get
    for (i = 0; i < psvr.peerlist.count; i++) {
        if(psvr.dl_num >= psvr.max_conn)
            break;
        if( NOT_GOOD_FOR_DOWNLOAD(&psvr.peerlist.peers[i]))
            continue;
        try_send_get(i);
    }
// if we don't reach maximum download limit and there's chunks left to download
    if(psvr.dl_num == 0 && psvr.dl_remain > 0
            && get_timestamp_now() - psvr.last_whohas > DEFAULT_TIMEOUT ) {
//   send whohas
        send_whohas();
        psvr.last_whohas = get_timestamp_now();
    }
}

// iterate each peer in connection and check timeout
int check_all_timeout() {
    int i;
    Peer *peer_p;

    for (i = 0; i < psvr.peerlist.count; i++) {
        peer_p = &(psvr.peerlist.peers[i]);

        if(! peer_p->is_alive )
            continue;

        if(peer_p->is_downloading
                && dl_check_timeout(&peer_p->dl) > MAX_TIMEOUT_CNT
                // and if download timeout
          )
        {
            // stop download activity
            kill_download(&peer_p->dl);
            // stop upload activity for peer i if existed
            if (peer_p->is_uploading) {
                kill_upload(&peer_p->ul);
            }

            die( peer_p );
            // find another one to download
        }
        if(peer_p->is_uploading
                && ul_check_timeout(&peer_p->ul) > MAX_TIMEOUT_CNT
                // and if upload timeout
          )
        {
            // stop upload activity
            kill_upload(&peer_p->ul);
            // stop download activity and find another one to download for peer i if existed
            if (peer_p->is_downloading) {
                kill_download(&peer_p->dl);
                // find another one to download
            }
            die( peer_p );
        }
    }

    // if any unfetched chunk exists, and we do not reach maximum download limit.
    // Then find another one to download (probably we need send whohas)
    refresh_chunk_download();

    return 0;
}

int addr2Index(struct sockaddr_in addr) {
    int i;
    PeerList *pl = (PeerList *) (&psvr.peerlist);

    for (i = 0; i < pl->count; i++) {
        if(pl->peers[i].addr.sin_port == addr.sin_port
                && pl->peers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr
          ) {
            return i;
        }
    }
    return -1;
}

/**
 * look up chunk by the hash
 * @return -1 if not find, otherwise return the index in the chunklist
 */
int hash2Index(ChunkList *clist, const char *hash) {
    int i;

    for (i = 0; i < clist->count; i++) {
        if (0 == strcmp(clist->chunks[i].sha1, hash)) {
            return i;
        }
    }

    return -1;
}

int write_winsize(int p_index, uint32_t window_size) {
    uint32_t now = get_timestamp_now();
    if (0 == now) {
        logger(LOG_ERROR, "get_timestamp_now() error");
        return -1;
    }

    fprintf(psvr.w_fp, "f%d\t%"PRIu32"\t%"PRIu32"\n", p_index, (now - psvr.start_ts), window_size);
    return fflush(psvr.w_fp);
}
