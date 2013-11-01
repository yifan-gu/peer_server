#include <sys/socket.h>
#include <netinet/in.h>

#include <download.h>
#include <upload.h>

typedef struct _Peer {
    struct sockaddr_in addr;

<<<<<<< HEAD
  download_t dl;
  upload_t ul;
=======
    int is_alive;
    int is_downloading;
    Download dl;
    int is_uploading;
    Upload ul;
>>>>>>> c378f0962b987bdeb41a88794dea2b109e8d3448

} Peer;

void die(Peer *pr);
