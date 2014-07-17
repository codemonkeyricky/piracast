// ========================================================
// Includes
// ========================================================

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include "sx_pkt.h"
#include "sx_desc.h"
#include "sx_pipe.h"
#include "sx_thread_priority.h"
#include "sx_audio_sink.h"

// ========================================================
// Constants
// ========================================================

// ========================================================
// Private Types
// ========================================================

typedef struct
{
    sSX_DESC   *head;
    sSX_DESC   *tail;

} sSLICE_CHAIN;

// Decoder control block
typedef struct
{
    pthread_t       decoder_thread;
    UINT32          look_for_new_slice;
    UINT32          continue_current_slice;
    UINT32          pes_len;
    UINT32          pes_curr_byte_count;
    UINT32          last_seq_num;
    sSLICE_CHAIN    slice_chain;

} sDECODER_CBLK;


// ========================================================
// Private Variables & Functions
// ========================================================

// Decoder control block.
static sDECODER_CBLK f_cblk;

static sSX_DESC * slice_get(
    void
    )
{
    sSX_DESC *head;


    head = f_cblk.slice_chain.head;

    f_cblk.slice_chain.head = NULL;
    f_cblk.slice_chain.tail = NULL;

//    printf("slice_get() invoked\n");

    return head;
}


static void slice_drop(
    void
    )
{
    sSX_DESC  *curr;
    sSX_DESC  *next;


    // Drop slice.
    sx_desc_put(f_cblk.slice_chain.head);

    // Reset head and tail.
    f_cblk.slice_chain.head = NULL;
    f_cblk.slice_chain.tail = NULL;

    printf("slice_dropped() invoked\n");
}


static void audio_decoder_slice_dump(
    sSX_DESC   *slice_head
    )
{
    sx_pipe_put(SX_VRDMA_LPCM_SLICE, slice_head);
}


static UINT32 pes_payload_size(
    sSX_DESC  *desc
    )
{
    UINT8  *curr_ptr;
    UINT32  bytes_left;
    UINT32  afc;
    UINT32  pid;
    UINT32  pes_byte_count;
    UINT32  payload_size;


    // Get current.
    curr_ptr = desc->data;

    // Get data left.
    bytes_left = desc->data_len;
    assert(bytes_left > sizeof(sRTP_HDR));

    // Get RTP header.
    // sRTP_HDR *rtp_hdr = (sRTP_HDR *) curr_ptr;

    // Get TS header.
    curr_ptr += sizeof(sRTP_HDR);

    // Get TS bytes left.
    bytes_left -= sizeof(sRTP_HDR);
    assert((bytes_left % sizeof(sMPEG2_TS)) == 0);

    pes_byte_count = 0;
    do
    {
        sMPEG2_TS *ts = (sMPEG2_TS *) curr_ptr;

        afc = AFC_GET(ts->hdr);
        pid = PID_GET(ts->hdr);

        if(pid == 0x1100)
        {
            if(PUSI_GET(ts->hdr))
            {
                // This makes Piracast to work with Sony Xperia
                // As it sends afc with payload (11b)
                assert((afc == 0x01) || (afc == 0x03));

                //            printf("### 0x%x 0x%x 0x%x 0x%x\n",
                //                   ts->payload.payload[14],
                //                   ts->payload.payload[15],
                //                   ts->payload.payload[16],
                //                   ts->payload.payload[17]);

                payload_size = (sizeof(sMPEG2_TS_PAYLOAD) - 20);
            }
            else
            {
                if(afc == 0x01)
                {
                    //                printf("### 2\n");
                    payload_size = sizeof(sMPEG2_TS_PAYLOAD);
                }
                else if(afc == 0x03)
                {
                    //                printf("### 3\n");
                    payload_size = sizeof(sMPEG2_TS_PAYLOAD) - 1 - ts->payload.payload[0];
                }
                else
                {
                    assert(0);
                }
            }

            pes_byte_count += payload_size;

//            printf("(pes_payload_size): payload_size = %d\n", payload_size);
        }

        curr_ptr += sizeof(sMPEG2_TS);
        bytes_left -= sizeof(sMPEG2_TS);

    } while (bytes_left > 0);

    return pes_byte_count;
}


static void slice_pkt_add(
    sSX_DESC   *desc
    )
{
    assert(desc != NULL);


    if(f_cblk.slice_chain.head == NULL)
    {
        f_cblk.slice_chain.head = desc;
        f_cblk.slice_chain.tail = desc;

        return;
    }

    // Append to tail.
    f_cblk.slice_chain.tail->next = desc;

    // Update tail.
    f_cblk.slice_chain.tail = desc;

//    printf("slice_pkt_add() invoked\n");
}


static UINT8 slice_start_find(
    sSX_DESC   *desc,
    UINT32     *pes_payload_size
    )
{
    UINT8      *curr_ptr;
    UINT32      bytes_left;
    sMPEG2_TS  *ts;
    UINT32      pid;
    UINT32      afc;


    // Get current.
    curr_ptr = desc->data;

    // Get data left.
    bytes_left = desc->data_len;
    assert(bytes_left > sizeof(sRTP_HDR));

    // Get RTP header.
    sRTP_HDR *rtp_hdr = (sRTP_HDR *) curr_ptr;

    // Get TS header.
    curr_ptr += sizeof(sRTP_HDR);

    // Get TS bytes left.
    bytes_left -= sizeof(sRTP_HDR);
    assert((bytes_left % sizeof(sMPEG2_TS)) == 0);

    do
    {
        sMPEG2_TS *ts = (sMPEG2_TS *) curr_ptr;

        afc = AFC_GET(ts->hdr);
        pid = PID_GET(ts->hdr);

        if(PUSI_GET(ts->hdr) && (pid == 0x1100))
        {
            curr_ptr = &ts->payload.payload[0];

            curr_ptr += sizeof(sPES);

            sPES_EXT *pes_ext = (sPES_EXT *) curr_ptr;

            *pes_payload_size = ntohs(pes_ext->length) - 14;

            // PES packet length is zero for Sony Xperia
            assert((*pes_payload_size == 1920) || (*pes_payload_size == -14));

            return 1;
        }

        curr_ptr += sizeof(sMPEG2_TS);
        bytes_left -= sizeof(sMPEG2_TS);

    } while (bytes_left > 0);

    return 0;
}


// --------------------------------------------------------
// decoder_thread
//      Decoder thread
//
//  Description:
//      This function defines the decoder thread.
//
void audio_decoder_thread(
    void * arg
    )
{
    sSX_DESC  *desc;
    UINT8      *curr_ptr;
    UINT32      bytes_left;
    sMPEG2_TS  *ts;

//    UINT32  last_seq_num = 0;
    UINT8   eos;


    while(1)
    {
        do
        {
            desc = sx_pipe_get(SX_VRDMA_LPCM);
            if(desc == NULL)
            {
                break;
            }

            // Get current.
            curr_ptr = desc->data;

            // Get data left.
            bytes_left = desc->data_len;
            assert(bytes_left > sizeof(sRTP_HDR));

            // Get RTP header.
            sRTP_HDR *rtp_hdr = (sRTP_HDR *) curr_ptr;

            if(f_cblk.look_for_new_slice)
            {
                if(slice_start_find(desc, &f_cblk.pes_len))
                {
                    f_cblk.look_for_new_slice = 0;

                    f_cblk.continue_current_slice = 1;

                    f_cblk.pes_curr_byte_count = 0;

                    f_cblk.last_seq_num = ntohs(rtp_hdr->sequence_num) - 1;
                }
            }

            if(f_cblk.continue_current_slice)
            {
                if(ntohs(rtp_hdr->sequence_num) != (f_cblk.last_seq_num + 1))
                {
//                    printf("(audio_decoder): rtp seq num = %d, last_seq_num = %d\n",
//                            ntohs(rtp_hdr->sequence_num),
//                            f_cblk.last_seq_num);

                    slice_drop();

                    f_cblk.continue_current_slice = 0;
                    f_cblk.look_for_new_slice = 1;
                }
                else
                {
                    f_cblk.last_seq_num = ntohs(rtp_hdr->sequence_num);

                    UINT32 pes_size = pes_payload_size(desc);

//                    assert(pes_size == 1920);

//                    printf("(audio_decoder): PES payload size = %d\n", pes_size);

                    f_cblk.pes_curr_byte_count += pes_size;

                    assert(f_cblk.pes_curr_byte_count <= f_cblk.pes_len);

//                    assert(0);

                    slice_pkt_add(desc);

                    if(f_cblk.pes_curr_byte_count == f_cblk.pes_len)
                    {
                        f_cblk.continue_current_slice = 0;
                        f_cblk.look_for_new_slice = 1;

//                        printf("(audio_decoder): Slice end found!\n");

                        audio_decoder_slice_dump(slice_get());
                    }
                }
            }

        } while (1);


//        printf("(audio_decoder): sleep!\n");

        usleep(1000);
    }
}


// --------------------------------------------------------
// common_resource_init
//      Initialize common resources
//
//  Description:
//      This function initializes the common resources.
//
static void resources_init(
    void
    )
{
    f_cblk.look_for_new_slice = 1;

    // Initialize decoder hardware.
    sx_audio_sink_init();
}


// --------------------------------------------------------
// decoder_thread_create
//      Creates decoder thread
//
//  Description:
//      This function creates decoder thread.
//
static void decoder_thread_create(
    void
    )
{
    sx_thread_create(&f_cblk.decoder_thread, &audio_decoder_thread, NULL, MGMT_AUDIO_DECODER_THREAD_PRIORITY);
}


// ========================================================
// Public Functions
// ========================================================

// --------------------------------------------------------
// sx_mgmt_audio_decoder_init
//      Initializes decoder
//
void sx_mgmt_audio_decoder_init(
    void
    )
{
    // Initialize common resources.
    resources_init();
}


// --------------------------------------------------------
// sx_mgmt_audio_decoder_open
//      Opens decoder
//
void sx_mgmt_audio_decoder_open(
    void
    )
{
    // Initialize decoder thread.
    decoder_thread_create();
}


// --------------------------------------------------------
// sx_mgmt_audio_decoder_close
//      Closes decoder
//
void sx_mgmt_audio_decoder_close(
    void
    )
{
}
