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
#include "sx_video_sink.h"

// ========================================================
// Constants
// ========================================================

// ========================================================
// Private Types
// ========================================================

// Decoder control block
typedef struct
{
    pthread_t       video_scheduler_thread;
    pthread_t       slice_packing_thread;
    pthread_t       pcr_update_thread;
    UINT64          curr_time;
    pthread_mutex_t lock;

} sMGMT_VIDEO_SCHEDULER_CBLK;


// ========================================================
// Private Variables & Functions
// ========================================================

// Decoder control block.
static sMGMT_VIDEO_SCHEDULER_CBLK f_cblk;

static void video_scheduler_slice_dump(
    sSX_DESC   *slice_head
    )
{
    UINT8      *curr_ptr;
    UINT32      bytes_left;
    UINT32      afc;
    UINT32      pes_byte_count;
    UINT32      payload_size;
    UINT32      start_offset;
    UINT32      copy_index;
    UINT32      pid;
    sSX_DESC   *curr_desc;
    sSX_DESC   *head_desc;


    // Consistency check.
    assert(slice_head);

    // Get descriptor.
    sSX_DESC *hw_desc = sx_desc_get();

    // Set this as another chain.
    sSX_DESC *hw_desc_head = hw_desc;

    // Get a hw buffer.
    sDECODER_HW_BUFFER * hw_buf = sx_video_sink_buf_get();
    assert(hw_buf != NULL);

    // Set payload.
    hw_desc->data = (UINT8 *) hw_buf;

    // Get first descriptor.
    //
    // First descriptor holds the timestamp and will be skipped.
    curr_desc = slice_head;

    copy_index = 0;
    do
    {
        // Get next.
        curr_desc = curr_desc->next;

        // Get current.
        curr_ptr = curr_desc->data;

        // Get data left.
        bytes_left = curr_desc->data_len;
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

            if(pid == 0x1011)
            {
                if(PUSI_GET(ts->hdr))
                {
                    assert(afc == 0x01);

                    start_offset = 14;

                    payload_size = (sizeof(sMPEG2_TS_PAYLOAD) - 14);
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

                        payload_size = sizeof(sMPEG2_TS_PAYLOAD) - 1 - ts->payload.payload[0];
                    }
                    else
                    {
                        assert(0);
                    }
                }

                if((copy_index + payload_size) > 81920)
                {
                    // If the hw buffer is full, just submit the current buffer.
                    hw_buf->buffer_len = copy_index;

                    // Get a new descriptor.
                    hw_desc->next = sx_desc_get();

                    // Point to the new descriptor.
                    hw_desc = hw_desc->next;

                    // Get a new buffer.
                    hw_buf = sx_video_sink_buf_get();
                    assert(hw_buf != NULL);

                    // Set new payload.
                    hw_desc->data = (UINT8 *) hw_buf;

                    // Reset index.
                    copy_index = 0;
                }

                memcpy(&hw_buf->buffer[copy_index],
                        &ts->payload.payload[start_offset],
                        payload_size);

                copy_index += payload_size;
            }

            curr_ptr += sizeof(sMPEG2_TS);
            bytes_left -= sizeof(sMPEG2_TS);

        } while (bytes_left > 0);

    } while (curr_desc->next != NULL);

    // Set length.
    hw_buf->buffer_len = copy_index;

    // Free the existing slice, minus the head (timestamp).
    sx_desc_put(slice_head->next);

    // Set slice.
    slice_head->next = hw_desc_head;

    sx_pipe_put(SX_VRDMA_SLICE_READY, slice_head);
}


/**
 * Get current time on the sink.
 *
 * @return
 */
static UINT64 sink_time_get(
    void
    )
{
    struct timeval  curr_time;


    // Get current time.
    gettimeofday(&curr_time, NULL);

    UINT64 temp = curr_time.tv_sec * 1000 + curr_time.tv_usec / 1000;

    return temp;
}


/**
 * Get estimated source time.
 *
 * @return
 */
static UINT64 estimated_source_time_get(
    void
    )
{
    UINT64  time;


    pthread_mutex_lock(&f_cblk.lock);

    time = f_cblk.curr_time;

    pthread_mutex_unlock(&f_cblk.lock);

    return time;
}


// --------------------------------------------------------
// decoder_thread
//      Decoder thread
//
//  Description:
//      This function defines the decoder thread.
//
void video_scheduler_thread(
    void * arg
    )
{
    sSX_DESC   *desc;
    UINT64      slice_present_time;


    desc = NULL;
    while(1)
    {
        while(1)
        {
            // -------------------
            // Get slice.
            // -------------------

            // Get slice.
            if(desc == NULL)
            {
                desc = sx_pipe_get(SX_VRDMA_SLICE_READY);
                if(desc == NULL)
                {
                    goto next_iter;
                }

                sSLICE_HDR *hdr = (sSLICE_HDR *) desc->data;

                // Get PTS.
                slice_present_time = ((sSLICE_HDR *) desc->data)->timestamp;
            }

            UINT64  estimated_source_time = estimated_source_time_get();
            UINT8 present = (estimated_source_time > slice_present_time) ? 1 : 0;

            if(!present)
            {
                goto next_iter;
            }

            // -------------------
            // Present slice.
            // -------------------

            sSX_DESC *curr = desc->next;
            sSX_DESC *next;
            do
            {
                sx_video_sink_buf_set((sDECODER_HW_BUFFER *) curr->data);

                next = curr->next;

                free(curr);

                curr = next;

            } while (curr != NULL);

            free(desc->data);
            free(desc);

            // Set descriptor pointer to NULL.
            desc = NULL;
        }

        next_iter:

        usleep(1*1000);
    }
}


/**
 * Slice packing thread.
 *
 * @param arg
 */
void slice_packing_thread(
    void * arg
    )
{
    sSX_DESC   *desc;


    desc = NULL;
    while(1)
    {
        UINT32 len = sx_pipe_len_get(SX_VRDMA_SLICE_READY);
        if(len >= 10)
        {
            // More than enough. Try again next iteration.
            goto next_iter;
        }

        UINT32  slices_to_dump = 10 - len;
        do
        {
            desc = sx_pipe_get(SX_VRDMA_SLICE);
            if(desc == NULL)
            {
                goto next_iter;
            }

            // Dump slice.
            video_scheduler_slice_dump(desc);

            slices_to_dump--;

        } while (slices_to_dump > 0);

        next_iter:

        usleep(2*1000);
    }
}


/**
 * Updates program clock reference time.
 *
 * @param arg
 */
void pcr_update_thread(
    void * arg
    )
{
    sSX_DESC   *desc;
    UINT64      pcr_time;
    UINT64      pcr_received_time;
    UINT64      curr_time;


    while(1)
    {
        desc = sx_pipe_get(SX_VRDMA_PCR);
        if(desc == NULL)
        {
            goto cleanup;
        }

        sSLICE_HDR *hdr = (sSLICE_HDR *) desc->data;

        // Update PCR time.
        pcr_time = hdr->timestamp;

        // Free descriptor.
        sx_desc_put(desc);

        // Cache received time.
        pcr_received_time = sink_time_get();

        cleanup:

        // Get current time.
        curr_time = sink_time_get();

        pthread_mutex_lock(&f_cblk.lock);

        f_cblk.curr_time = pcr_time + (curr_time - pcr_received_time) - SX_SYSTEM_AUDIO_SOURCE_DELAY_MS - SX_SYSTEM_DELAY_MS;

        pthread_mutex_unlock(&f_cblk.lock);

        usleep(2*1000);
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
    sx_video_sink_init();

    pthread_mutex_init(&f_cblk.lock, NULL);
}


// --------------------------------------------------------
// decoder_thread_create
//      Creates decoder thread
//
//  Description:
//      This function creates decoder thread.
//
static void video_scheduler_thread_create(
    void
    )
{
    sx_thread_create(&f_cblk.video_scheduler_thread,
                     &video_scheduler_thread,
                     NULL,
                     VIDEO_SCHEDULER_THREAD_PRIORITY);

    sx_thread_create(&f_cblk.slice_packing_thread,
                     &slice_packing_thread,
                     NULL,
                     VIDEO_SCHEDULER_THREAD_PRIORITY);

    sx_thread_create(&f_cblk.pcr_update_thread,
                     &pcr_update_thread,
                     NULL,
                     VIDEO_SCHEDULER_THREAD_PRIORITY);
}


// ========================================================
// Public Functions
// ========================================================

void sx_mgmt_video_scheduler_init(
    void
    )
{
    // Initialize common resources.
    resources_init();
}


// --------------------------------------------------------
// decoder_open
//      Opens decoder
//
void sx_mgmt_video_scheduler_open(
    void
    )
{
    // Initialize decoder thread.
    video_scheduler_thread_create();
}


void sx_mgmt_video_scheduler_close(
    void
    )
{
}
