#include <stdio.h> 
#include <string.h> 
#include <assert.h> 

#include <sys/socket.h>
#include <netinet/in.h> 
#include <errno.h> 

// #include "ws_capture.c"

#define UINT8 unsigned char 
#define UINT16 unsigned short
#define UINT32 unsigned int

typedef struct
{
    UINT8   version_p_x_cc; 
    UINT8   m_pt; 
    UINT16  sequence_num; 
    UINT32  timestamp; 
    UINT32  ssrc_id; 

} sRTP_HDR; 


#define TRANSPORT_ERROR_INDICATOR       0x80
#define PAYLOAD_UNIT_START_INDICATOR    0x40
#define TRANSPORT_PRIORITY              0x20
#define PID_1                           0x1F
#define PID_2                           0xFF

#define PID_GET(hdr)    ((((hdr).tei_pusi_tp_pid1 & 0x1F) << 8) | (hdr).pid2)

#define PUSI_GET(hdr)    (((hdr).tei_pusi_tp_pid1 & 0x40) >> 6) 

#define AFC_GET(hdr)    (((hdr).tsc_afc_cc & 0x30) >> 4) 

typedef struct
{
    UINT8   sync_byte;          ///< Sync byte
    UINT8   tei_pusi_tp_pid1;   ///< Transport Error Indicate (1 bit)
                                ///< Payload Unit Start Indicator (1 bit)
                                ///< Transport Priority (1 bit)
                                ///< PID (first half, 5 bit)
    UINT8   pid2;               ///< PID (second half, 8 bit)
    UINT8   tsc_afc_cc;         ///< Transport scrambling control (2 bits)
                                ///< Adaption Field Control (2 bits)
                                ///< Continuity Counter (4 bits)

} sMPEG2_TS_HDR; 


typedef struct
{
    UINT8   payload[184]; 

} sMPEG2_TS_PAYLOAD; 


typedef struct
{
    sMPEG2_TS_HDR       hdr; 
    sMPEG2_TS_PAYLOAD   payload; 

} sMPEG2_TS; 

static UINT8 f_buffer[1024*64]; 

static UINT8 f_packet[1024*2]; 

typedef struct
{
    char   *payload; 
    UINT32  payload_len; 
} sPAYLOAD; 


typedef struct
{
    unsigned int    peer_ip;
    unsigned short  peer_port;
    unsigned int    local_ip;
    unsigned short  local_port;

    int             sock;

} sUDP_CBLK;


int main()
{
    UINT8  *curr_ptr; 
    UINT32  pkt_size; 
    UINT32  bytes_left; 
    UINT32  index; 
    sMPEG2_TS  *ts; 
    UINT32      pid; 
    UINT32  afc; 
    UINT32  copy_len; 
    UINT32  copy_index; 
    UINT32  iter;


    iter = 0; 
    index = 0; 

    FILE * write_ptr = fopen("output.h264", "wb"); 


    struct sockaddr_in  client_addr;
    struct sockaddr_in  server_addr;
    int                 rv;
    int     sock; 
    UINT32  bytes_read; 
    UINT32  addr_len;


    // Create socket.
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(sock != -1);

    char *opt; 
    opt = "wlan0"; 
    rv = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, opt, 5); 
    assert(rv == 0); 

    // Listen to anyone that talks to us on the right port.
    client_addr.sin_family      = AF_INET;
    client_addr.sin_port        = htons(50000); 
    client_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(client_addr.sin_zero), 8);

    // Bind to address.
    rv = bind(sock,
            (struct sockaddr *) &client_addr,
            sizeof(client_addr)); 
    if(rv < 0)
    {
        printf("(udp): udp_socket_create(): rv = %d, errno %s\n", rv, strerror(errno));
        assert(0); 
    }


    while(1)
    {
#if 0
        curr_ptr = packets[iter].payload; 
        bytes_left = packets[iter].payload_len; 
        iter++; 

        printf("Packet %d\n", iter); 

        if(iter >= PACKET_LIST_LEN)
        {
            break; 
        }
#endif 

        printf("Waiting for packet...\n"); 

        addr_len = sizeof(struct sockaddr);
        int pkt_len = 2048; 
        bytes_read = recvfrom(sock,
                              f_packet,
                              pkt_len,
                              0,
                              (struct sockaddr *) &server_addr,
                              &addr_len);
        assert(bytes_read > 0); 

        curr_ptr = f_packet; 
        bytes_left = bytes_read; 

        printf("Packet %d received\n", iter++); 

        // Advance past RTP header. 
        curr_ptr += sizeof(sRTP_HDR); 
        bytes_left -= sizeof(sRTP_HDR);

#if 1
        do
        {
            ts = (sMPEG2_TS *) curr_ptr; 

            // printf("0x%x 0x%x\n", ts->hdr.tei_pusi_tp_pid1, ts->hdr.pid2);

            pid = PID_GET(ts->hdr); 

            // printf("pid = 0x%x\n", pid); 

            if(pid == 0x1011)
            {
                if(PUSI_GET(ts->hdr))
                {
                    index = 0; 
                }

                afc = AFC_GET(ts->hdr); 

                if(afc == 0x01)
                {
                    copy_len = sizeof(sMPEG2_TS_PAYLOAD); 

                    copy_index = 0; 
                }
                else if(afc == 0x03)
                {
                    copy_len = sizeof(sMPEG2_TS_PAYLOAD) - 1 - ts->payload.payload[0]; 

                    copy_index = 1 + ts->payload.payload[0]; 

#if 0
                    printf("copy_len = %d, copy_index = %d, struct_size = %d, payload = %d\n", 
                           copy_len, 
                           copy_index, 
                           sizeof(sMPEG2_TS_PAYLOAD),
                           ts->payload.payload[4]); 
#endif
                }
                else
                {
                    assert(0); 
                }

                printf("index = %d, copy_len = %d\n", index, copy_len); 

                memcpy(&f_buffer[index], &ts->payload.payload[copy_index], copy_len); 

                index += copy_len; 
            }
            else
            {
                if(index > 0)
                {
                    printf("PES len = %d, NAL len = %d\n", index, index - 14); 

                    UINT32 offset = 14; 

                    printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
                            f_buffer[offset + 0], 
                            f_buffer[offset + 1], 
                            f_buffer[offset + 2], 
                            f_buffer[offset + 3], 
                            f_buffer[offset + 4], 
                            f_buffer[offset + 5], 
                            f_buffer[offset + 6], 
                            f_buffer[offset + 7]); 

                    fwrite(&f_buffer[offset], index - offset, 1, write_ptr); 

                    index = 0; 
                }

                // Don't care for now. 
            }

            // printf("bytes_left = %d\n", bytes_left); 

            curr_ptr += sizeof(sMPEG2_TS); 
            bytes_left -= sizeof(sMPEG2_TS);

        } while (bytes_left > 0); 
#endif 
    } 

    fclose(write_ptr); 
}
