#include <sys/socket.h>
#include <netinet/in.h>

#include <download.h>
#include <upload.h>

typedef struct _Peer {
  struct sockaddr_in addr;

  download_t dl;
  upload_t ul;

} Peer;
