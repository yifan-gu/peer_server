#include <sys/socket.h>
#include <netinet/in.h>

#include <download.h>
#include <upload.h>

typedef struct _Peer {
    struct sockaddr_in addr;

    Linlist hasqueue;

    int is_alive;
    int is_downloading;
    Download dl;
    int is_uploading;
    Upload ul;
} Peer;

void die(Peer *pr);
