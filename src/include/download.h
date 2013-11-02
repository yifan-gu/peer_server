#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <linkedlist.h>

#include "packet.h"
#include <bt_parse.h>
#include "util.h"
#include "bitmap.h"

/**
 * the max receiver's window size
 */
#define RECV_WINDOW_SIZE 512
//#define FIXED_DATA_LENGTH 1024

/**
 * number of acks to send when timeouts
 */
#define LOSS_ACK_NUM 3

#define GET_BITMAP_OFFSET(dl, seq) ((dl)->data_length * (seq - 1))

#define REVERSE_BITMAP_OFFSET(dl, i) ((i) / (dl)->data_length + 1)

typedef struct download_s {
    /**
     * The chunk index in getchunks
     */
    int get_index;
    int p_index;

    /**
     * timeout:
     * - last sent ack timestamp
     */
    uint32_t ts;
    int timeout_cnt;
    
    /**
     * next seq number
     * receive window
     */
    uint32_t next_pkt_expected;
    Recvbm bm;
    
    /**
     * file
     */
    char filename[BT_FILENAME_LEN];
    
    /**
     * rtt dev
     */
    uint32_t rtt;
    uint32_t dev;

    /**
     * flags
     */
    int started;
    int block_update;
    int finished;

    /**
     * the data
     */
    char buffer[BT_CHUNK_SIZE];
    size_t data_length;
}download_t;

/**
 * alias
 */
typedef download_t Download;


/**
 * init the receiver handler
 * @param dl, the handler
 * @param p_index, the peer index
 * @param get_index, the chunk index
 * @param filename, the target filename
 * @return 0 on success, -1 if fails
 */
int dl_init(download_t *dl, int p_index, int get_index, const char *filename);

/**
 * receive the packet, save it into buffer
 * send corresponding ack back
 * @param dl, the handler
 * @param pkt, the received packet
 * @return 0 on success, -1 if fails
 */
int dl_recv(download_t *dl, packet_t *pkt);

/**
 * dl_check_timeout
 * @return Number of timeout times. -1 if the connection is finished
 */
int dl_check_timeout(download_t *dl);

/**
 * check the hach
 * @return 0 on success, -1 if the hash not equal
 */
int dl_check_hash(void);

/**
 * a wrapper for finishing the downloading
 * @param dl, the handler
 * @return 0 on success, 1 if check_hash fails, -1 if other IO fails
 */
//int dl_finish(download_t *dl);

/**
 * write the buffer to file
 * @return 0 on success -1 if fails
 */
int dl_save_buffer(download_t *dl);

/**
 * a debugging helper
 * @param dl, the handler
 */
void dl_dump(download_t *dl, FILE *fp);

#endif // for #ifndef _DOWNLOAD_H
