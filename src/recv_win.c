/*
@brief
  This module provides receive window relevant functions and attributes.
 */
#include <recv_win.h>

#define MOD_WIN(i) ((i) % RECV_WINDOW_SIZE)
#define SEQ_IN_WIN(seq, rwin) MOD_WIN((seq) - (rwin)->next_seq + (rwin)->front)

void init_recvwin(RecvWin *rwin) {
    BM_CLR(rwin->bm);
    rwin->front = 0;
    rwin->next_seq = 1;
}

// whether the sequence number can be suitably accepted within receive window.
// @return
//  1 if yes. 0 if no.
int seq_fit_in(RecvWin *rwin, uint32_t seq) {
    return seq >= rwin->next_seq
           && seq < (rwin->next_seq + RECV_WINDOW_SIZE);
}

// whether the packet of specific sequence number exists in receive window already
// @return
//  1 if yes. 0 if no.
int seq_exist_in(RecvWin *rwin, uint32_t seq) {
    return BM_ISSET(rwin->bm, SEQ_IN_WIN(seq, rwin) );
}

void recvwin_mark(RecvWin *rwin, uint32_t seq) {
    BM_SET(rwin->bm, SEQ_IN_WIN(seq, rwin) );
}

// We have receive a lot of data packets so we can slide the ack pointer forward a few steps.
void recvwin_slideack(RecvWin *rwin) {
    uint32_t size, seq;

    for (seq = rwin->next_seq, size = 0;
            size < RECV_WINDOW_SIZE;
            size++, seq++) {
        if( BM_ISSET(rwin->bm, SEQ_IN_WIN(seq, rwin)) ) {
            BM_UNSET(rwin->bm, SEQ_IN_WIN(seq, rwin) );
        }
        else
            break;
    }
    rwin->front = SEQ_IN_WIN(seq, rwin);
    rwin->next_seq += size;
}
