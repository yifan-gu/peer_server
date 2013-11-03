#ifndef _CHUNKLIST_H
#define _CHUNKLIST_H

#include <sha.h>

#define MAX_CHUNK_NUM 4096

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

int parse_chunk(ChunkList *cl, char *chunk_list_file) ;
void add_chunk(ChunkList *clist, ChunkLine *cline);
ChunkLine* new_chunkline() ;
void delete_chunkline(void *cl);
#endif // for #ifndef _CHUNKLIST_H
