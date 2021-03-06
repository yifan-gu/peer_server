#include <parse_packet.h>
#include <peer_server.h>
#include <logger.h>
#include <download.h>
#include <upload.h>
#include <send_helper.h>

extern PeerServer psvr;

/*
@brief
  The state machine handles received packets' logistics based on their types.
  We build a sophisticated state machine to handle different types of packets
  and provide the logic/workflow to process them.
  We have settled most of timeout and concurrency issues here and the algorithm is
  detailedly discussed in readme.
@param
  pkt: received packet parsed object;
  peer_addr: peer's socket address;
@return
  If succeeded, 0;
  If failed, -1;
 */
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
        printf("Peer[%d] is asking for chunks\n", peer_p->id);
        if(psvr.ul_num >= psvr.max_conn){
            printf("Exceeding max connections[%d], won't reply\n", psvr.max_conn);
            break;
        }
        
        if( ! peer_p->is_alive ){
            peer_p->is_alive = 1;
        }
        if( peer_p->is_uploading ) {
            printf("Exceeding max connections[%d], won't reply\n", psvr.max_conn);
            break;
        }
        
        logger(LOG_DEBUG, "receive whohas\n");
        send_ihave(pkt, p_index);
        break;

    case PACKET_TYPE_IHAVE:
        logger(LOG_DEBUG, "alive: %d, dowloading %d\n", peer_p->is_alive, peer_p->is_downloading);
        if( ! peer_p->is_alive ){
            peer_p->is_alive = 1;
        }

        if( peer_p->is_downloading )
            break;

        // parse the ihave packet chunks into the peer's corresponding information
        if(parse_download(pkt, p_index) < 0) {
            logger(LOG_ERROR, "Failed in parsing IHAVE packet from %s:%d!\n",
                   inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
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
        peer_p->is_uploading = 1;
        break;

    case PACKET_TYPE_DATA:
        if( ! peer_p->is_alive || ! peer_p->is_downloading )
            break;

        logger(LOG_DEBUG, "Receive data: %d", GET_SEQ(pkt));
        print_download_progress(peer_p);
        // update download
        update_download(&peer_p->dl, pkt);
        // if last packet:
        //  finish download
        if ( is_download_finished(&peer_p->dl)) {
            /*finish_download(&peer_p->dl);*/
            psvr.dl_num --;
            logger(LOG_DEBUG, "Finish download chunk %d", psvr.dl_remain);
            peer_p->is_downloading = 0;
            // if hash check succeed
            //   write to file
            if(check_hash_succeed(&peer_p->dl)){
                write_to_file(&peer_p->dl);
            }
            // if we didn't successfully get it fetched
            if(psvr.getchunks.chunks[peer_p->dl.get_index].state != fetched)
                psvr.getchunks.chunks[peer_p->dl.get_index].state = unfetched;

            // find another one to download
            if (psvr.dl_remain > 0) {
                refresh_chunk_download();
            }
        }
        
        //  find another one to download
        break;

    case PACKET_TYPE_ACK :
        if( ! peer_p->is_alive || ! peer_p->is_uploading )
            break;

        logger(LOG_DEBUG, "Receive ack: %d", GET_ACK(pkt));
        // update upload
        update_upload(&peer_p->ul, pkt);
        //ul_dump(&peer_p->ul, log_fp);
        // if last ack:
        //  finish upload
        if (is_upload_finished(&peer_p->ul)) {
            finish_upload(&peer_p->ul);
            peer_p->is_uploading = 0;
        }

        break;
    default:
        break;
    }

    return 0;
}

