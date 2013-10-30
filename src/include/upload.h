#ifndef _UPLOAD_H
#define _UPLOAD_H

typedef struct _Upload {
    // The chunk index in haschunks
    int hasIndex;
    int p_index;

    // duplicate acks: last ack number and counts
    //
    // last sent data timestamp
    //
    // congestion control:
    // - ssthresh
    // - last ack number
    // - current window size
    // - rtt
}Upload;

// ul_check_timeout
// @return Number of timeout times. -1 if the connection is finished

#endif // for #ifndef _UPLOAD_H
