#ifndef _SEND_HELPER_H
#define _SEND_HELPER_H


void parse_ihavechunks(packet_t *pkt, int);
int parse_download(packet_t *pkt, int);
int send_get(int, int getIndex);

/**
 * start download
 */
int start_download(Download *dl, int p_index, int get_index, const char *filename);

/**
 * update download info when get a DATA
 */
int update_download(Download *dl, packet_t *pkt);

/**
 * finish download
 */
int finish_download(Download *dl);

/**
 * kill download
 */
int kill_download(Download *dl);

/**
 * check if download is OK
 * @return 1 if finished, 0 if not, -1 if hash is not correct
 */
int is_download_finished(Download *ul);
    
/**
 * start the upload
 */
int start_upload(Upload *ul, int p_index, int has_index);

/**
 * update upload info when get an ACK
 */
int update_upload(Upload *ul, packet_t *pkt);

/**
 * check if upload is finished
 */
int is_upload_finished(Upload *ul);

/**
 * finish upload
 */
int finish_upload(Upload *ul);

#define kill_upload(ul) (finish_upload((ul)))

#endif // for #ifndef _SEND_HELPER_H
