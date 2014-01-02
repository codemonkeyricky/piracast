#if !defined(PI_PORTAL_PKT)
#define PI_PORTAL_PKT

#include "sx_types.h"

#define TRANSPORT_ERROR_INDICATOR       0x80
#define PAYLOAD_UNIT_START_INDICATOR    0x40
#define TRANSPORT_PRIORITY              0x20
#define PID_1                           0x1F
#define PID_2                           0xFF

#define PID_GET(hdr)                    ((((hdr).tei_pusi_tp_pid1 & 0x1F) << 8) | (hdr).pid2)
#define CC_GET(hdr)                     ((hdr).tsc_afc_cc & 0x0F)
#define PUSI_GET(hdr)                   (((hdr).tei_pusi_tp_pid1 & 0x40) >> 6)
#define AFC_GET(hdr)                    (((hdr).tsc_afc_cc & 0x30) >> 4)

#define PI_PORTAL_PKT_HDR_SIZE          sizeof(unsigned int)
#define PI_PORTAL_PKT_PAYLOAD_SIZE_MAX  (1472-PI_PORTAL_PKT_HDR_SIZE)

typedef struct
{
    UINT8   version_p_x_cc;
    UINT8   m_pt;
    UINT16  sequence_num;
    UINT32  timestamp;
    UINT32  ssrc_id;

} sRTP_HDR;


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


typedef struct
{
    UINT8   prefix1;
    UINT8   prefix2;
    UINT8   prefix3;
    UINT8   stream;

} sPES;


typedef struct
{
    UINT16  length;
    UINT8   flag1;
    UINT8   flag2;

} sPES_EXT;


typedef struct
{
    UINT8   hdr_len;

} sPES_EXT2;


typedef struct
{
    unsigned int    hdr;
    unsigned char   payload[PI_PORTAL_PKT_PAYLOAD_SIZE_MAX];

} sPI_PORTAL_PKT;


#define SLICE_TYPE_PCR      0
#define SLICE_TYPE_SLICE    1

typedef struct
{
    UINT8   type;
    UINT8   rsvd[3];
    UINT64  timestamp;

} sSLICE_HDR;

#endif
