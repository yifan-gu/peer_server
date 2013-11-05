/*
@brief
  This module provides functions to handle chunk list logistics.
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <chunklist.h>
#include <logger.h>


/*
@Assumtpion:
  We have a maximum chunk number limit. Lines larger than that will be discarded.
@brief:
  Parse the chunk file logic into easily accessible code structure.
@param
  cl: chunk list object reference;
  chunk_list_file: chunk list file;
@return
  If succeeded, 0;
  If failed, -1;
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

// Add one chunk line into the chunk list object
void add_chunk(ChunkList *clist, ChunkLine *cline){
  int i;
  for (i = 0; i < clist->count; i++) {
      if(strcmp(clist->chunks[i].sha1, cline->sha1) == 0)
          return;
  }
  if(i < clist->count){
      clist->chunks[i] = *cline;
      clist->count ++;
  }
}

ChunkLine* new_chunkline() {
    return malloc(sizeof(ChunkLine));
}

void delete_chunkline(void *cl) {
    free(cl);
}
