#include <sys/socket.h>
#include <netinet/in.h>

#include <download.h>

typedef struct _Peer {
  struct sockaddr_in addr;
  Download dl;
} Peer;
