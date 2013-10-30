#ifndef _BITMAP_H
#define _BITMAP_H

#include <chunk.h>

#define FIXED_PAYLOAD_SIZE 1024
#define BM_NUM (BT_CHUNK_SIZE/FIXED_PAYLOAD_SIZE/32)

#define BM_CLR(bm) memset((bm).a, 0, BM_NUM * sizeof(int))
#define BM_SET(bm, i) (bm).a[(i)/32] |= (1 << ((i) % 32) )
#define BM_ISSET(bm, i) (!!( (bm).a[(i)/32] & (1 << ((i) % 32) ) ) )

typedef struct _Recvbm {
  int a[BM_NUM];
} Recvbm;

#endif // for #ifndef _BITMAP_H
