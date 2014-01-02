// ========================================================
// Includes
// ========================================================

#include <stdio.h>
#include <pthread.h>

#include "sx_desc.h"
#include "sx_udp.h"
#include "sx_pkt.h"
#include "sx_thread.h"
#include "sx_pipe.h"

#include "sx_mgmt_data.h"

// ========================================================
// Constants
// ========================================================

// ========================================================
// Private Types
// ========================================================

// --------------------------------------------------------
// sMGMT_DATA_CBLK
//      Control block structure.
//
typedef struct
{
    pthread_t       pkt_rx_thread;  ///< Packet receive thread
    SBOX_UDP_ID     udp_sock;       ///< UDP socket
    unsigned int    rx_pkt;         ///< Number of received packet

} sMGMT_DATA_CBLK;

// ========================================================
// Private Variables & Functions
// ========================================================

// Define local control block.
static sMGMT_DATA_CBLK  f_cblk;

// --------------------------------------------------------
// pkt_rx_thread
//      Create packet receive thread.
//
static void pkt_rx_thread(
    void * arg
    )
{
    sPI_PORTAL_PKT *pkt;
    UINT16          last_seq_num;
    UINT16          curr_seq_num;


    while(1)
    {
        // Malloc packet to hold data.
        pkt = malloc(sizeof(sPI_PORTAL_PKT));

        // Receive packet.
        UINT32 pkt_len = sizeof(sPI_PORTAL_PKT);

        // -----------------------------
        //  Wait for packet.
        // -----------------------------

        sx_udp_recv(f_cblk.udp_sock, (char *) pkt, &pkt_len);

        // -----------------------------
        //  Check for missing packets.
        // -----------------------------

        sRTP_HDR *rtp_hdr = (sRTP_HDR *) pkt;

        curr_seq_num = ntohs(rtp_hdr->sequence_num);
        if((UINT16) (last_seq_num + 1) != curr_seq_num)
        {
            printf("(mgmt_data): last_seq_num = %u, curr_seq_num = %u, lost %u pkts.\n",
                    last_seq_num,
                    curr_seq_num,
                    curr_seq_num - (last_seq_num + 1));
        }

        // Cache last sequence number.
        last_seq_num = curr_seq_num;

        // -----------------------------
        //  Push packet to process queue.
        // -----------------------------

        sSX_DESC *desc = sx_desc_get();

        desc->data      = (void *) pkt;
        desc->data_len  = pkt_len;

        sx_pipe_put(SX_VRDMA_PKT_QUEUE, desc);

        f_cblk.rx_pkt++;
    }
}


// --------------------------------------------------------
// data_rx_thread_create
//      Create data RX thread
//
static void pkt_rx_thread_create(
    void
    )
{
    sx_thread_create(&f_cblk.pkt_rx_thread, &pkt_rx_thread, NULL, MGMT_DATA_RX_THREAD_PRIORITY);
}


// ========================================================
// Public Functions
// ========================================================

// --------------------------------------------------------
// sx_mgmt_data_init
//      Initialize data manager.
//
void sx_mgmt_data_init( 
    void
    )
{
    f_cblk.udp_sock = sx_udp_create(50000);

    printf("(mgmt_data): mgmt_data_init(): Initialized.\n");
}


// --------------------------------------------------------
// sx_mgmt_data_open
//      Open data manager on session boundary.
//
void sx_mgmt_data_open( 
    void
    )
{
    pkt_rx_thread_create();

    printf("(mgmt_data_open): Opened.\n");
}

