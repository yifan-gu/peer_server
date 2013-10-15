#ifndef _PEER_SERVER_H
#define _PEER_SERVER_H

#include <bt_parse.h>
#include <peerlist.h>
#include <sha.h>

#define MAX_CHUNK_NUM 1024

enum ChunkState{
  unfetched, fetching, fetched
};

typedef struct _ChunkLine{
  enum ChunkState state; //
  int id;
  char sha1[SHA1_HASH_SIZE*2 + 1];
}ChunkLine;

typedef struct _ChunkList {
  int count;
  ChunkLine chunks[MAX_CHUNK_NUM];
}ChunkList;


int peer_init(bt_config_t *config);
int parse_chunk(ChunkList *, char *);

#endif // for #ifndef _PEER_SERVER_H
