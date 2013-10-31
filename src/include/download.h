#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

#include <sys/socket.h>
#include <netinet/in.h>

#include <linkedlist.h>


typedef struct _Download {
    Linlist queue;

    // The chunk index in getchunks
    int getIndex;
    int p_index;

    // timeout:
    // - last sent ack timestamp
    //
    // next seq number
    // receive window
    //
    // file
    //
    // rtt

}Download;

// dl_check_timeout
// @return Number of timeout times. -1 if the connection is finished

int init_download(Download *dl);

int find_unfetched_chunk(int);

// find another one to download

#endif // for #ifndef _DOWNLOAD_H
