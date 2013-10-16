/*
 * unit test for chunk.h/c
 */

#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "assert.h"

int main(int argc, char *argv[])
{
    char buf[MAX_LINE_SIZE*2];
    char raw[MAX_LINE_SIZE*2];
    Linlist chunks_I_want;
    ll_Node *i;
    char *get_chunk_file = "/home/yifan/cmu/network/proj2/peer_server/example/A.chunks";
    FILE *fp = fopen(get_chunk_file, "r");
    
    init_linkedlist(&chunks_I_want);
    printf("chunks_i want %p\n", &chunks_I_want.head);
    
    printf("getting chunk list from the local file\n");
    assert(0 == get_chunks(get_chunk_file, &chunks_I_want));

    for (i = ll_start(&chunks_I_want); i != ll_end(&chunks_I_want); i = ll_next(i)) {
        printf("pointer: %p", i);
        printf("%d: %s\n", ((chunk_t *)i->item)->id, ((chunk_t *)i->item)->hash);
        printf("length: %lu\n", strlen(((chunk_t *)i->item)->hash));
        sprintf(buf, "%d %s", ((chunk_t *)i->item)->id, ((chunk_t *)i->item)->hash);
        fgets(raw, MAX_LINE_SIZE*2, fp);

        printf("testing hash value\n");
        printf("%s\n%s", buf, raw);
        assert(0 == strncmp(buf, raw, SHA1_HASH_STR_SIZE));
    }

    printf("deleting chunk list\n");
    printf("chunks_i want %p\n", &chunks_I_want.head);
    ll_delete_allnodes(&chunks_I_want, delete_chunk);

    printf("PASS\n");
    
    fclose(fp);
    
    return 0;
}
