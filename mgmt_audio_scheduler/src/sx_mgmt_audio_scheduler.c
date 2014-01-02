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
#include <netinet/in.h>

#include "sx_system.h"
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

typedef enum
{
    STATE_INACTIVE,
    STATE_ACTIVE

} eSTATE;

// Decoder control block
typedef struct
{
    pthread_t   audio_scheduler_thread;
    eSTATE      state;

} sMGMT_AUDIO_SCHEDULER_CBLK;


// ========================================================
// Private Variables & Functions
// ========================================================

// Decoder control block.
static sMGMT_AUDIO_SCHEDULER_CBLK f_cblk;


static UINT32 audio_total_remaining_ms(
    void
    )
{

    UINT32  data_left = sx_pipe_len_get(SX_VRDMA_LPCM_SLICE) * 10;
    UINT32  queued_left = sx_audio_sink_ms_left_get();

//    printf("total left = %u, data left = %u, queued left = %u\n",
//            data_left + queued_left,
//            data_left,
//            queued_left);

    return (data_left + queued_left);
}


static void inline audio_endianness_convert(
    UINT16 *temp,
    UINT32  samples
    )
{
    UINT32 i;


    for(i = 0; i < samples; i++)
    {
        temp[i] = ntohs(temp[i]);
    }
}


static void audio_scheduler_slice_dump(
    sSX_DESC  *slice_head
    )
{
    UINT8      *curr_ptr;
    UINT32      bytes_left;
    UINT32      afc;
    UINT32      pid;
    UINT32      pes_byte_count;
    UINT32      payload_size;
    UINT32      start_offset;
    UINT32      copy_index;
    UINT32      samples_left;
    UINT32      ms_left;
    sSX_DESC   *desc;


    static UINT8   playback_speed = 1;

    ms_left = audio_total_remaining_ms();
    if(ms_left > (100 + SX_SYSTEM_DELAY_MS))
    {
        if(playback_speed != 2)
        {
            sx_audio_sink_playback_speed_inc();

            playback_speed = 2;
        }
    }
    else if(ms_left < (50 + SX_SYSTEM_DELAY_MS))
    {
        if(playback_speed != 0)
        {
            sx_audio_sink_playback_speed_dec();

            playback_speed = 0;
        }
    }
    else
    {
        if(playback_speed != 1)
        {
            sx_audio_sink_playback_speed_reset();

            playback_speed = 1;
        }
    }

    UINT8 *buf = sx_audio_sink_buffer_get();

    copy_index = 0;

    desc = slice_head;
    do
    {
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
                    assert(afc == 0x01);

                    start_offset = 20;

                    payload_size = (sizeof(sMPEG2_TS_PAYLOAD) - start_offset);
                }
                else
                {
                    if(afc == 0x01)
                    {
                        start_offset = 0;

                        payload_size = sizeof(sMPEG2_TS_PAYLOAD);
                    }
                    else if(afc == 0x03)
                    {
                        start_offset = 1 + ts->payload.payload[0];

                        payload_size = sizeof(sMPEG2_TS_PAYLOAD) - start_offset;
                    }
                    else
                    {
                        assert(0);
                    }
                }

//                printf("copy 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",
//                        ts->payload.payload[start_offset],
//                        ts->payload.payload[start_offset + 1],
//                        ts->payload.payload[start_offset + 2],
//                        ts->payload.payload[start_offset + 3],
//                        ts->payload.payload[start_offset + 4],
//                        ts->payload.payload[start_offset + 5],
//                        ts->payload.payload[start_offset + 6],
//                        ts->payload.payload[start_offset + 7]);

                memcpy(&buf[copy_index],
                        &ts->payload.payload[start_offset],
                        payload_size);

                copy_index += payload_size;
            }

            curr_ptr += sizeof(sMPEG2_TS);
            bytes_left -= sizeof(sMPEG2_TS);

        } while (bytes_left > 0);

        desc = desc->next;

    } while (desc != NULL);

    sx_desc_put(slice_head);

    assert(copy_index == 1920);

//    printf("### 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
//            buf[0],
//            buf[1],
//            buf[2],
//            buf[3],
//            buf[4],
//            buf[5],
//            buf[6],
//            buf[7]);

    audio_endianness_convert((UINT16 *) buf, 960);

    // Push to decoder hardware.
    sx_audio_sink_buffer_set(buf, 1920);
}


// --------------------------------------------------------
// decoder_thread
//      Decoder thread
//
//  Description:
//      This function defines the decoder thread.
//
void audio_scheduler_thread(
    void * arg
    )
{
    sSX_DESC   *desc;


    while(1)
    {
        if(f_cblk.state == STATE_INACTIVE)
        {
            UINT32 len = sx_pipe_len_get(SX_VRDMA_LPCM_SLICE);
            if(len > (SX_SYSTEM_DELAY_MS / 10))
            {
                f_cblk.state = STATE_ACTIVE;

                printf("(audio_scheduler): Transition to active. [len = %u]\n", len);

                goto next_iter;
            }
        }
        else
        {
            UINT32  data_left_ms = sx_pipe_len_get(SX_VRDMA_LPCM_SLICE) * 10;
            UINT32  queued_ms = sx_audio_sink_ms_left_get();

            if((data_left_ms + queued_ms) == 0)
            {
                f_cblk.state = STATE_INACTIVE;

                printf("(audio_scheduler): Transition to inactive.\n");

                goto next_iter;
            }

            if(queued_ms < 200)
            {
                UINT32  slices_to_queue = (200 - queued_ms + 10) / 10;
                do
                {
                    // Get slice.
                    desc = sx_pipe_get(SX_VRDMA_LPCM_SLICE);
                    if(desc == NULL)
                    {
                        goto next_iter;
                    }

                    // Dump slice.
                    audio_scheduler_slice_dump(desc);

                    slices_to_queue--;

                } while(slices_to_queue > 0);
            }
        }

        next_iter:

        usleep(5*1000);
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
static void audio_scheduler_thread_create(
    void
    )
{
    sx_thread_create(&f_cblk.audio_scheduler_thread,
                     &audio_scheduler_thread,
                     NULL,
                     MGMT_AUDIO_SCHEDULER_THREAD_PRIORITY);
}


// ========================================================
// Public Functions
// ========================================================

void sx_mgmt_audio_scheduler_init(
    void
    )
{
    // Initialize common resources.
    resources_init();

    printf("(audio_scheduler): Init.\n");
}


// --------------------------------------------------------
// decoder_open
//      Opens decoder
//
void sx_mgmt_audio_scheduler_open(
    void
    )
{
    // Initialize decoder thread.
    audio_scheduler_thread_create();

    printf("(audio_scheduler): Open.\n");
}


void sx_mgmt_audio_scheduler_close(
    void
    )
{
}
