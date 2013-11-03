#ifndef _RECV_WIN_H
#define _RECV_WIN_H

#include <string.h>
#include <stdint.h>

#define RECV_WINDOW_SIZE (1024)
#define INT_SIZE 32
#define INT_BYTES 4
#define BM_SIZE (RECV_WINDOW_SIZE/INT_SIZE)

#define BM_CLR(bm) memset((bm), 0, BM_SIZE * INT_BYTES)
#define BM_SET(bm, i) (bm)[(i)/INT_SIZE] |= (1 << ((i) % INT_SIZE) )
#define BM_UNSET(bm, i) (bm)[(i)/INT_SIZE] ^= (1 << ((i) % INT_SIZE) )
#define BM_ISSET(bm, i) (!!( (bm)[(i)/INT_SIZE] & (1 << ((i) % INT_SIZE) ) ) )

typedef struct _RecvWin {
  int bm[BM_SIZE];
  int front;
  uint32_t next_seq;
} RecvWin;

void init_recvwin(RecvWin *rwin);
int seq_fit_in(RecvWin *rwin, uint32_t);
int seq_exist_in(RecvWin *rwin, uint32_t);
void recvwin_mark(RecvWin *rwin, uint32_t);
void recvwin_slideack(RecvWin *rwin);

#endif // for #ifndef _RECV_WIN_H
