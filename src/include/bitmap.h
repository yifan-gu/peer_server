#ifndef _BITMAP_H
#define _BITMAP_H

#define BM_NUM (512/32)

#define BM_CLR(bm) memset((bm).a, 0, BM_NUM * sizeof(int))
#define BM_SET(bm, i) (bm).a[(i)/32] |= (1 << ((i) % 32) )
#define BM_ISSET(bm, i) (!!( (bm).a[(i)/32] & (1 << ((i) % 32) ) ) )

typedef struct _Recvbm {
  int a[BM_NUM];
} Recvbm;

#endif // for #ifndef _BITMAP_H
