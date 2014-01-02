// ========================================================
// Includes
// ========================================================
#include <stdio.h>
#include <assert.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "sx_mgmt_env.h"
#include "sx_video_sink.h"

// ========================================================
// Constants
// ========================================================

// ========================================================
// Private Types
// ========================================================

// ========================================================
// Private Variables & Functions
// ========================================================

static COMPONENT_T *video_decode; 
static COMPONENT_T *video_scheduler; 
static COMPONENT_T *video_render; 
static COMPONENT_T *video_clock; 
static COMPONENT_T *comp_list[5]; 
static TUNNEL_T tunnel[4];
static ILCLIENT_T *client;

static unsigned int port_setting_changed = 0; 
static unsigned int first_packet = 1; 

//#define WRITE_TO_FILE 1

#if WRITE_TO_FILE
static FILE * write_ptr;
#endif

// ========================================================
// Public Functions
// ========================================================

// --------------------------------------------------------
// decoder_hw_init
//      Initializes decoder hardware. 
//
void sx_video_sink_init(
    void 
    )
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    int rc; 


    memset(comp_list, 0, sizeof(comp_list));
    memset(tunnel, 0, sizeof(tunnel));

    // Initialize Broadcom host. 
    bcm_host_init();

    // initialize il client. 
    client = ilclient_init(); 
    assert(client != NULL); 

    // initialize omx. 
    rc = OMX_Init(); 
    assert(rc == OMX_ErrorNone); 

    // Create decoder component. 
    rc = ilclient_create_component(client, 
                                   &video_decode,
                                   "video_decode",
                                   ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS);
    assert(rc == 0); 

    comp_list[0] = video_decode; 

    // Create video renderer. 
    rc = ilclient_create_component(client, 
                                   &video_render,
                                   "video_render",
                                   ILCLIENT_DISABLE_ALL_PORTS);
    assert(rc == 0); 

    comp_list[1] = video_render; 

    // Create clock.
    rc = ilclient_create_component(client, 
                                   &video_clock,
                                   "clock",
                                   ILCLIENT_DISABLE_ALL_PORTS);
    assert(rc == 0); 

    comp_list[2] = video_clock; 

    // Configure clock. 
    memset(&cstate, 0, sizeof(cstate)); 
    cstate.nSize                = sizeof(cstate);
    cstate.nVersion.nVersion    = OMX_VERSION;
    cstate.eState               = OMX_TIME_ClockStateWaitingForStartTime;
    cstate.nWaitMask            = 1;

    rc = OMX_SetParameter(ILC_GET_HANDLE(video_clock), 
                          OMX_IndexConfigTimeClockState,
                          &cstate);
    assert(rc == 0); 

    // Create clock. 
    rc = ilclient_create_component(client, 
                                   &video_scheduler,
                                   "video_scheduler",
                                   ILCLIENT_DISABLE_ALL_PORTS);
    assert(rc == 0); 

    comp_list[3] = video_scheduler; 

    // Set tunnels. 

    // Connect decode to scheduler. 
    set_tunnel(tunnel, video_decode, 131, video_scheduler, 10); 

    // Connect scheduler to renderer. 
    set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90); 

    // Connect clock to scheduler. 
    set_tunnel(tunnel+2, video_clock, 80, video_scheduler, 12); 

    // Setup clock tunnel first. 
    rc = ilclient_setup_tunnel(tunnel+2, 0, 0); 
    assert(rc == 0); 

    // Kick start the clock. 
    ilclient_change_component_state(video_clock, OMX_StateExecuting); 

 #define AUTO_FULLSCREEN
//
//    UINT32 sess_width = sx_mgmt_env_get(MGMT_ENV_VAR_SESSION_WIDTH);
//    UINT32 sess_height = sx_mgmt_env_get(MGMT_ENV_VAR_SESSION_HEIGHT);

#if 1
    OMX_CONFIG_DISPLAYREGIONTYPE drt;
    memset(&drt, 0, sizeof(drt));
    drt.nVersion.nVersion   = OMX_VERSION;
    drt.nSize               = sizeof(drt);
    drt.nPortIndex          = 90;
#if !defined(AUTO_FULLSCREEN)
    drt.src_rect.x_offset   = 0;
    drt.src_rect.y_offset   = 0;
    drt.src_rect.width      = sx_mgmt_env_get(MGMT_ENV_VAR_SESSION_WIDTH);
    drt.src_rect.height     = sx_mgmt_env_get(MGMT_ENV_VAR_SESSION_HEIGHT);
    drt.dest_rect.x_offset  = -56;
    drt.dest_rect.y_offset  = 0;
    drt.dest_rect.width     = 1792;
    drt.dest_rect.height    = 1050;
#endif 
#if !defined(AUTO_FULLSCREEN)
    drt.fullscreen          = OMX_FALSE;
#else
    drt.fullscreen          = OMX_TRUE;
#endif
    drt.noaspect            = OMX_TRUE;
    drt.mode = OMX_DISPLAY_MODE_FILL;

#if !defined(AUTO_FULLSCREEN)
    drt.set = (OMX_DISPLAYSETTYPE) (  OMX_DISPLAY_SET_SRC_RECT |
                                      OMX_DISPLAY_SET_DEST_RECT |
                                      OMX_DISPLAY_SET_FULLSCREEN |
                                      OMX_DISPLAY_SET_NOASPECT);
#else
    drt.set = (OMX_DISPLAYSETTYPE) (  OMX_DISPLAY_SET_FULLSCREEN |
                                      OMX_DISPLAY_SET_NOASPECT);
#endif

    rc = OMX_SetConfig(ILC_GET_HANDLE(video_render), OMX_IndexConfigDisplayRegion, &drt);
    assert(rc == 0); 
#endif

    // Kick start video decoder. 
    ilclient_change_component_state(video_decode, OMX_StateIdle); 

    // Configure decoder. 
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE)); 
    format.nSize                = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion    = OMX_VERSION;
    format.nPortIndex           = 130;
    format.eCompressionFormat   = OMX_VIDEO_CodingAVC;

    rc = OMX_SetParameter(ILC_GET_HANDLE(video_decode), 
                          OMX_IndexParamVideoPortFormat,
                          &format);
    assert(rc == 0); 

    // Enable video decode. 
    rc = ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL); 
    assert(rc == 0); 

    // Kick start video decoder. 
    ilclient_change_component_state(video_decode, OMX_StateExecuting);

#if WRITE_TO_FILE
    write_ptr = fopen("video_sink.h264", "wb");
#endif

    printf("decoder_hw: initialized");
}


// --------------------------------------------------------
// decoder_hw_get_buf
//      Get decoder hardware buffer. 
//
sDECODER_HW_BUFFER * sx_video_sink_buf_get(
    void
    )
{
    OMX_BUFFERHEADERTYPE *buf;


    // Attempt to get a buffer. 
    buf = ilclient_get_input_buffer(video_decode, 130, 0); 

    // Typecase to our overlay and return. 
    return (sDECODER_HW_BUFFER *) buf; 
}


// --------------------------------------------------------
// decoder_hw_set_buf
//      Set decoder hardware buffer. 
//
void sx_video_sink_buf_set(
    sDECODER_HW_BUFFER *decoder_hw_buf
    )
{
#define MAX_BYTES_ALLOWED   (16 << 10)
    OMX_BUFFERHEADERTYPE *buf;
    int                     rc; 


    buf = (OMX_BUFFERHEADERTYPE *) decoder_hw_buf; 

#if WRITE_TO_FILE
    fwrite(decoder_hw_buf->buffer, decoder_hw_buf->buffer_len, 1, write_ptr);
#endif

    if(    !port_setting_changed 
       && (ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0))
    {
        printf("(decoder_hw): port setting changed!\n"); 

        port_setting_changed = 1; 

        // Setup tunnel for ... 
        rc = ilclient_setup_tunnel(tunnel, 0, 0); 
        assert(rc == 0); 

        // Kick start scheduler. 
        ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

        rc = ilclient_setup_tunnel(tunnel+1, 0, 1000); 
        assert(rc == 0); 

        // Kick start renderer. 
        ilclient_change_component_state(video_render, OMX_StateExecuting);
    }

    // Set buffer property. 
    buf->nOffset = 0; 
    buf->nFlags = first_packet ? OMX_BUFFERFLAG_STARTTIME : OMX_BUFFERFLAG_TIME_UNKNOWN; 
    first_packet = 0; 

    // FLush into the decoder. 
    rc = OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf); 
    assert(rc == 0); 
}

