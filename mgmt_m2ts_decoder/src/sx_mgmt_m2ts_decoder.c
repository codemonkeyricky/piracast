// ========================================================
// Includes
// ========================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#include "sx_udp.h"
#include "sx_pkt.h"
#include "sx_desc.h"
#include "sx_thread.h"
#include "sx_queue.h"

#include "sx_types.h"
#include "sx_pipe.h"

// ========================================================
// Constants
// ========================================================

// #define LOOPBACK

// ========================================================
// Private Types
// ========================================================

typedef struct
{
    pthread_t   pkt_process_thread;

} sMGMT_DATA_CBLK;


// ========================================================
// Private Variables & Functions
// ========================================================

static sMGMT_DATA_CBLK  f_cblk;

typedef enum
{
    PKT_TYPE_AUDIO,
    PKT_TYPE_VIDEO,
    PKT_TYPE_NULL

} ePKT_TYPE;


static UINT64 pcr_get(
    sSX_DESC   *desc
    )
{
    UINT8  *curr_ptr;
    UINT32  bytes_left;

    static UINT64   last_pcr_ms;


    // Get current.
    curr_ptr = desc->data;

    // Get data left.
    bytes_left = desc->data_len;
    assert(bytes_left > sizeof(sRTP_HDR));

    // Get TS header.
    curr_ptr += sizeof(sRTP_HDR);

    // Get TS bytes left.
    bytes_left -= sizeof(sRTP_HDR);
    assert((bytes_left % sizeof(sMPEG2_TS)) == 0);

    do
    {
        sMPEG2_TS *ts = (sMPEG2_TS *) curr_ptr;
        UINT16 pid = PID_GET(ts->hdr);

        if(pid == 0x1000)
        {
            curr_ptr += (sizeof(sMPEG2_TS_HDR) + 2);

            UINT64  pcr = 0;
            UINT32  i;
            for(i = 0; i < 6; i++)
            {
                pcr = ((pcr << 8) | curr_ptr[i]);
            }

//            printf("(pcr_get): curr_ptr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
//                    curr_ptr[0],
//                    curr_ptr[1],
//                    curr_ptr[2],
//                    curr_ptr[3],
//                    curr_ptr[4],
//                    curr_ptr[5]);
//
//            printf("(pcr_get): pcr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
//                    ((UINT8 *) &pcr)[0],
//                    ((UINT8 *) &pcr)[1],
//                    ((UINT8 *) &pcr)[2],
//                    ((UINT8 *) &pcr)[3],
//                    ((UINT8 *) &pcr)[4],
//                    ((UINT8 *) &pcr)[5]);

            UINT64 pcr_base = (pcr >> (9 + 6));

            UINT64 pcr_ext = pcr & (0x1FF);

            pcr = pcr_base * 300 + pcr_ext;

            UINT64  pcr_ms = pcr / 27000;

//            printf("(pcr_get): pcr = %llu, pcr_ms = %llu, delta = %u\n",
//                    pcr,
//                    pcr_ms,
//                    (UINT32) (pcr_ms - last_pcr_ms));

            last_pcr_ms = pcr_ms;

            return pcr_ms;
        }

        bytes_left -= sizeof(sMPEG2_TS);
        curr_ptr += sizeof(sMPEG2_TS);

    } while (bytes_left > 0);

    return 0;
}


static ePKT_TYPE pkt_type_get(
    sSX_DESC   *desc
    )
{
    UINT8  *curr_ptr;
    UINT32  bytes_left;


    // Get current.
    curr_ptr = desc->data;

    // Get data left.
    bytes_left = desc->data_len;
    assert(bytes_left > sizeof(sRTP_HDR));

    // Get TS header.
    curr_ptr += sizeof(sRTP_HDR);

    // Get TS bytes left.
    bytes_left -= sizeof(sRTP_HDR);
    assert((bytes_left % sizeof(sMPEG2_TS)) == 0);

    // printf("(mgmt_m2ts): pkt_type_get() invoked!\n");

    do
    {
        sMPEG2_TS *ts = (sMPEG2_TS *) curr_ptr;
        UINT16 pid = PID_GET(ts->hdr);

#if 0
        printf("(mgmt_m2ts): 0x%x 0x%x 0x%x 0x%x\n",
                ts->hdr.sync_byte,
                ts->hdr.tei_pusi_tp_pid1,
                ts->hdr.pid2,
                ts->hdr.tsc_afc_cc);

        printf("(mgmt_m2ts): pid = 0x%.4x\n", pid);
#endif

        if(pid == 0x1011)
        {
            return PKT_TYPE_VIDEO;
        }
        else if(pid == 0x1100)
        {
            return PKT_TYPE_AUDIO;
        }

        bytes_left -= sizeof(sMPEG2_TS);
        curr_ptr += sizeof(sMPEG2_TS);

    } while (bytes_left > 0);

    return PKT_TYPE_NULL;
}


static void pkt_process_thread(
    void * arg
    )
{
    SX_QUEUE   *queue;
    sSX_DESC   *desc;
    sSX_DESC   *h264_desc;
    sSX_DESC   *lpcm_desc;
    UINT32      bytes_left;


    while(1)
    {
        do
        {
            desc = sx_pipe_get(SX_VRDMA_PKT_QUEUE);
            if(desc == NULL)
            {
                break;
            }

            // Get data left.
            bytes_left = desc->data_len;
            assert(bytes_left > sizeof(sRTP_HDR));

            // ------------------------
            //  Get and push program reference time.
            // ------------------------

            UINT64 pcr_ms = pcr_get(desc);
            if(pcr_ms > 0)
            {
                sSX_DESC *new_desc = sx_desc_get();

                sSLICE_HDR *hdr = malloc(sizeof(sSLICE_HDR));
                assert(hdr != NULL);

                hdr->type       = SLICE_TYPE_PCR;
                hdr->timestamp  = pcr_ms;

                new_desc->data = (void *) hdr;
                new_desc->data_len = sizeof(sSLICE_HDR);

                sx_pipe_put(SX_VRDMA_PCR, new_desc);
            }

            // ------------------------
            //  Get and push media packet.
            // ------------------------

            ePKT_TYPE pkt_type = pkt_type_get(desc);
            switch(pkt_type)
            {
                case PKT_TYPE_VIDEO:
                {
                    sx_pipe_put(SX_VRDMA_VIDEO_PKT_QUEUE, desc);
                    break;
                }
                case PKT_TYPE_AUDIO:
                {
                    sx_pipe_put(SX_VRDMA_LPCM, desc);
                    break;
                }
                case PKT_TYPE_NULL: 
                {
                    sx_desc_put(desc); 
                    break;
                }
                default:
                {
                    assert(0);
                    break;
                }
            }

        } while(1);

        usleep(1*1000);
    }
}


static void pkt_process_thread_create(
    void
    )
{
    sx_thread_create(&f_cblk.pkt_process_thread, &pkt_process_thread, NULL, MGMT_M2TS_PKT_PROCESS_THREAD_PRIORITY);
}


// ========================================================
// Public Functions
// ========================================================

// --------------------------------------------------------
// hole_punching_client_init
//      Opens decoder
//
void sx_mgmt_m2ts_decoder_init(
    void
    )
{
    printf("(mgmt_m2ts): sx_mgmt_m2ts_decoder_init(): Initialized.\n");
}


// --------------------------------------------------------
// mgmt_data_open
//      Open data manager on session boundary.
//
void sx_mgmt_m2ts_decoder_open(
    void
    )
{
    pkt_process_thread_create();

    printf("(mgmt_m2ts_decoder_open): sx_mgmt_m2ts_decoder_open(): Invoked.\n");
}
