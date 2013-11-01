#include <peer_server.h>
#include <stdio.h>
#include <malloc.h>
#include <peerlist.h>

#include <logger.h>
#include "send_helper.h"

#define BUFFER 256

PeerList peerlist;
FILE *master_chunk;
ChunkList haschunks;
ChunkList getchunks;
ChunkList ihavechunks;
int max_conn;
int dl_num;
int ul_num;

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

    init_peerlist(&peerlist, config->peers, config->identity);

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

    master_chunk = fopen(file_str, "r");
    if(master_chunk == NULL) {
        logger(LOG_ERROR, "can't open (%s) for master_chunk reading", file_str);
        return -1;
    }
    free(file_str);

    // has_chunk_file
    if(parse_chunk(&haschunks, config->has_chunk_file) < 0 ) {
        return -1;
    }
    getchunks.count = 0;

    // max_conn;
    max_conn = config->max_conn;
    dl_num = 0;
    ul_num = 0;
    return 0;
}

/*
Assumtpion:
  We have a maximum chunk number limit. Lines larger than that will be discarded.
 */
int parse_chunk(ChunkList *cl, char *chunk_list_file) {
    FILE *tmp_fp;
    int i;

    tmp_fp = fopen(chunk_list_file, "r");
    if(tmp_fp == NULL) {
        logger(LOG_ERROR, "can't open chunk list file: %s", chunk_list_file);
        return -1;
    }
    {
        i = 0;
        while(fscanf(tmp_fp, "%d %s", &cl->chunks[i].id, cl->chunks[i].sha1) != EOF) {
            cl->chunks[i].state = unfetched;
            i ++ ;
            if(i == MAX_CHUNK_NUM) {
                break;
            }
        }
        cl->count = i;
    }
    fclose(tmp_fp);

    return 0;
}

ChunkLine* new_chunkline() {
    return malloc(sizeof(ChunkLine));
}

void delete_chunkline(void *cl) {
    free(cl);
}

int addr2Index(struct sockaddr_in addr) {
    int i;
    PeerList *pl = &peerlist;

    for (i = 0; i < pl->count; i++) {
        if(pl->peers[i].addr.sin_port == addr.sin_port
                && pl->peers[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr
          ) {
            return i;
        }
    }
    return -1;
}

// iterate each peer in connection and check timeout
int check_all_timeout() {
    int i;
    Peer *peer_p;
    
    for (i = 0; i < peerlist.count; i++) {
        peer_p = &peerlist.peers[i];
        if(! peer_p->is_alive )
            continue;

        if(peer_p->is_downloading
           && dl_check_timeout(&peer_p->dl) > 3
           // and if download timeout
          )
        {
            // stop download activity
            // stop upload activity for peer i if existed
            if (peer_p->is_uploading) {
                kill_upload(&peer_p->ul);
            }
            
            die( &peerlist.peers[i] );
            // find another one to download
        }
        if(peer_p->is_uploading
           && ul_check_timeout(&peer_p->ul) > 3
                // and if upload timeout
          )
        {
            // stop upload activity
            kill_upload(&peer_p->ul);
            if (peer_p->is_downloading) {
                kill_download(&peer_p->dl);
            }
            // stop download activity and find another one to download for peer i if existed
            die( &peerlist.peers[i] );
        }
    }

    // if any unfetched chunk exists, and we do not reach maximum download number.
    // Then find another one to download (probably we need send whohas)

    return 0;
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
