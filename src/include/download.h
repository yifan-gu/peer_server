#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

#include <sys/socket.h>
#include <netinet/in.h>

#include <linkedlist.h>


typedef struct _Download {
    Linlist queue;
    int getIndex;
}Download;

int init_download(Download *dl);

int find_unfetched_chunk(int);

#endif // for #ifndef _DOWNLOAD_H
