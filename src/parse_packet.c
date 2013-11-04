#include <parse_packet.h>
#include <peer_server.h>
#include <logger.h>
#include <download.h>
#include <send_helper.h>

extern PeerServer psvr;

int parse_packet(packet_t *pkt, struct sockaddr_in peer_addr) {
    int p_index;
    int hasIndex;
    Peer *peer_p;
    char hash_buf[SHA1_HASH_SIZE*2+1];

    p_index = addr2Index(peer_addr);
    if(p_index < 0){
        return -1;
    }

    peer_p = & psvr.peerlist.peers[p_index];

    switch (GET_TYPE(pkt)) {

    case PACKET_TYPE_WHOHAS:
        if( ! peer_p->is_alive ){
            peer_p->is_alive = 1;
        }
        if( peer_p->is_uploading )
            break;
        logger(LOG_DEBUG, "receive whohas\n");
        send_ihave(pkt, p_index);
        break;

    case PACKET_TYPE_IHAVE:
        if( ! peer_p->is_alive ){
            peer_p->is_alive = 1;
        }

        if( peer_p->is_downloading )
            break;

        // parse the ihave packet chunks into the peer's corresponding information
        if(parse_download(pkt, p_index) < 0) {
            logger(LOG_ERROR, "Failed in parsing IHAVE packet from %s:%d!\n"
                   , inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
            return -1;
        }

        // check maximum download constraint
        if(psvr.dl_num >= psvr.max_conn){
            break;
        }

        try_send_get(p_index);
        break;

    case PACKET_TYPE_GET :
        if( ! peer_p->is_alive ){
            break;
        }

        // check maximum upload constraint
        if(psvr.ul_num >= psvr.max_conn){
            break;
        }

        logger(LOG_DEBUG, "Receive get");

        GET_HASH(pkt, 0, hash_buf);
        hasIndex = hash2Index(&psvr.haschunks, hash_buf);
        if (hasIndex < 0) {
            logger(LOG_INFO, "Invalid hash");
            break;
        }

        if( peer_p->is_uploading ){
            // Stop the previous upload.
            // This is useful when the downloading peer time out
            // and restart downloading (another chunk) again.
            kill_upload(&peer_p->ul);
        }
        //
        // start peer upload
        start_upload(&peer_p->ul, p_index, hasIndex);
        psvr.ul_num ++;
        peer_p->is_uploading = 1;
        break;

    case PACKET_TYPE_DATA:
        if( ! peer_p->is_alive || ! peer_p->is_downloading )
            break;

        logger(LOG_DEBUG, "Receive data: %d", GET_SEQ(pkt));
        // update download
        update_download(&peer_p->dl, pkt);
        // if last packet:
        //  finish download
        if ( is_download_finished(&peer_p->dl)) {
            logger(LOG_DEBUG, "Finish download");
            /*finish_download(&peer_p->dl);*/
            psvr.dl_num --;
            peer_p->is_downloading = 0;
            // if hash check succeed
            //   write to file
            if(check_hash_succeed(&peer_p->dl)){
                write_to_file(&peer_p->dl);
            }
            // find another one to download
            refresh_chunk_download();
        }

        //  find another one to download
        break;

    case PACKET_TYPE_ACK :
        if( ! peer_p->is_alive || ! peer_p->is_uploading )
            break;

        logger(LOG_DEBUG, "Receive ack: %d", GET_ACK(pkt));
        // update upload
        update_upload(&peer_p->ul, pkt);
        // if last ack:
        //  finish upload
        if (is_upload_finished(&peer_p->ul)) {
            finish_upload(&peer_p->ul);
            psvr.ul_num --;
            peer_p->is_uploading = 0;
        }

        break;
    default:
        break;
    }

    return 0;
}

