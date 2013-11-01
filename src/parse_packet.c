#include <parse_packet.h>
#include <peerlist.h>
#include <logger.h>
#include <download.h>
#include <send_helper.h>

extern PeerList peerlist;
extern bt_config_t config;
extern ChunkList getchunks;
int parse_packet(packet_t *pkt, struct sockaddr_in peer_addr) {
    int p_index = addr2Index(peer_addr);
    int getIndex, hasIndex;
    Peer *peer_p;
    char hash_buf[SHA1_HASH_SIZE*2+1];

    if(p_index < 0)
        return -1;

    peer_p = &peerlist.peers[p_index];
    if( ! peer_p->is_alive )
        return -1;

    
    switch (GET_TYPE(pkt)) {

    case PACKET_TYPE_WHOHAS:
        parse_ihavechunks(pkt, p_index);
        break;

    case PACKET_TYPE_IHAVE:
        if( ! peer_p->is_alive ){
            peer_p->is_alive = 1;
        }
        if( peer_p->is_downloading )
            break;

        if(parse_download(pkt, p_index) < 0) {
            logger(LOG_ERROR, "Failed in parsing IHAVE packet from %s:%d!\n"
                   , inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
            return -1;
        }

        // check maximum download constraint
        getIndex = find_unfetched_chunk(p_index);
        if(getIndex >= 0) {
            send_get(p_index, getIndex);
            // start peer download
            start_download(&peer_p->dl,
                           p_index, getIndex, config.output_file);
        }
        break;

    case PACKET_TYPE_GET :
        // check maximum upload constraint
        
        // check the validation of hash
        GET_HASH(pkt, 0, hash_buf);
        hasIndex = hash2Index(&getchunks, hash_buf);
        if (hasIndex < 0) {
            //printf("invalid hash\n");
            break;
        }
        
        if( peer_p->is_uploading ){
            // kill upload
            kill_upload(&peer_p->ul);
        }
        //
        // start peer upload
        start_upload(&peer_p->ul, p_index, hasIndex);

        printf("HAHAHA\n");
        break;

    case PACKET_TYPE_DATA:
        if(! peer_p->is_downloading )
            break;

        // update download
        update_download(&peer_p->dl, pkt);
        // if last packet:
        //  finish download
        if (0 == is_download_finished(&peer_p->dl)) {
            finish_download(&peer_p->dl);
            // fetched?
        }
        
        //  find another one to download
        break;

    case PACKET_TYPE_ACK :
        if(! peer_p->is_uploading)
            break;

        // update upload
        update_upload(&peer_p->ul, pkt);
        // if last ack:
        //  finish upload
        if (is_upload_finished(&peer_p->ul)) {
            finish_upload(&peer_p->ul);
        }
        
        break;
    default:
        break;
    }

    return 0;
}

