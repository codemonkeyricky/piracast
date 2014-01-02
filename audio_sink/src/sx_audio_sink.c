// ========================================================
// Includes
// ========================================================
#include <stdio.h>
#include <assert.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "sx_types.h"
#include "sx_audio_sink.h"

//#define WRITE_TO_FILE 1

#if WRITE_TO_FILE
static FILE * write_ptr;
#endif

// ========================================================
// Constants
// ========================================================

// Sample rate.
#define AUDIO_CODEC_SAMPLE_RATE     48000

// Number of channels.
#define AUDIO_CODEC_NUM_CHANNEL     2

// Number of samples per frame.
//  480 samples per frame at 8KHz is 60ms frame.
//  960 samples per frame at 16KHz is also 60ms frame.
#define AUDIO_CODEC_FRAME_SIZE      480

// 16 bits per sample.
#define AUDIO_CODEC_BIT_DEPTH       16

#define AUDIO_RENDERER              "local" // audio jack
//#define AUDIO_RENDERER              "hdmi"  // hdmi

// ========================================================
// Private Types
// ========================================================

typedef struct {
    sem_t                   sema;
    ILCLIENT_T             *client;
    COMPONENT_T            *audio_render;
    COMPONENT_T            *clock;
    COMPONENT_T            *list[2];
    OMX_BUFFERHEADERTYPE   *user_buffer_list; // buffers owned by the client
    UINT32                  num_buffers;
    UINT32                  bytes_per_sample;
    TUNNEL_T                tunnel;

} AUDIOPLAY_STATE_T;


// ========================================================
// Private Variables & Functions
// ========================================================

static AUDIOPLAY_STATE_T *st;

// ========================================================
// Public Functions
// ========================================================

static void input_buffer_callback(
    void           *data,
    COMPONENT_T    *comp
    )
{
   // do nothing - could add a callback to the user
   // to indicate more buffers may be available.
}


static INT32 audioplay_create(
    AUDIOPLAY_STATE_T **handle,
    UINT32              sample_rate,
    UINT32              num_channels,
    UINT32              bit_depth,
    UINT32              num_buffers,
    UINT32              buffer_size
    )
{
    OMX_AUDIO_PARAM_PCMMODETYPE pcm;
    UINT32   bytes_per_sample = (bit_depth * num_channels) >> 3;
    INT32   ret = -1;
    int rv;

    *handle = NULL;

    // basic sanity check on arguments
    if(sample_rate >= 8000 && sample_rate <= 96000 &&
       (num_channels == 1 || num_channels == 2 || num_channels == 4 || num_channels == 8) &&
       (bit_depth == 16 || bit_depth == 32) &&
       num_buffers > 0 &&
       buffer_size >= bytes_per_sample)
    {
        // buffer lengths must be 16 byte aligned for VCHI
        int size = (buffer_size + 15) & ~15;

        // buffer offsets must also be 16 byte aligned for VCHI
        st = calloc(1, sizeof(AUDIOPLAY_STATE_T));
        assert(st != NULL);

        OMX_ERRORTYPE error;
        OMX_PARAM_PORTDEFINITIONTYPE param;
        INT32   s;

        ret = 0;
        *handle = st;

        // create and start up everything
        s = sem_init(&st->sema, 0, 1);
        assert(s == 0);

        st->bytes_per_sample = bytes_per_sample;
        st->num_buffers = num_buffers;

        st->client = ilclient_init();
        assert(st->client != NULL);

        ilclient_set_empty_buffer_done_callback(st->client, input_buffer_callback, st);

        error = OMX_Init();
        assert(error == OMX_ErrorNone);

        // ------------------
        // Create audio render.
        // ------------------

        ilclient_create_component(st->client,
                                  &st->audio_render,
                                  "audio_render",
                                  ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_DISABLE_ALL_PORTS);
        assert(st->audio_render != NULL);

        st->list[0] = st->audio_render;

        // ------------------
        // Create clock.
        // ------------------

        ilclient_create_component(st->client,
                                  &st->clock,
                                  "clock",
                                  ILCLIENT_DISABLE_ALL_PORTS);
        assert(st->clock != NULL);

        st->list[1] = st->clock;

        // ------------------
        // Configure clock.
        // ------------------

        // Configure clock.
        OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;

        memset(&cstate, 0, sizeof(cstate));
        cstate.nSize                = sizeof(cstate);
        cstate.nVersion.nVersion    = OMX_VERSION;
        cstate.eState               = OMX_TIME_ClockStateWaitingForStartTime;
        cstate.nWaitMask            = 1;

        error = OMX_SetParameter(ILC_GET_HANDLE(st->clock),
                                 OMX_IndexConfigTimeClockState,
                                 &cstate);
        assert(error == 0);

        // ------------------
        // Connect clock and audio render.
        // ------------------

        set_tunnel(&st->tunnel, st->clock, 80, st->audio_render, 101);

        rv = ilclient_setup_tunnel(&st->tunnel, 0, 0);
        assert(rv == 0);

        // ------------------
        // Kick off clock.
        // ------------------

        ilclient_change_component_state(st->clock, OMX_StateExecuting);

        // ------------------
        // Set audio render port definition.
        // ------------------

        // set up the number/size of buffers
        memset(&param, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        param.nSize             = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
        param.nVersion.nVersion = OMX_VERSION;
        param.nPortIndex        = 100;

        error = OMX_GetParameter(ILC_GET_HANDLE(st->audio_render), OMX_IndexParamPortDefinition, &param);
        assert(error == OMX_ErrorNone);

        param.nBufferSize           = size;
        param.nBufferCountActual    = num_buffers;

        error = OMX_SetParameter(ILC_GET_HANDLE(st->audio_render), OMX_IndexParamPortDefinition, &param);
        assert(error == OMX_ErrorNone);

        // ------------------
        // Set audio render PCM definition.
        // ------------------

        memset(&pcm, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
        pcm.nSize               = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
        pcm.nVersion.nVersion   = OMX_VERSION;
        pcm.nPortIndex          = 100;
        pcm.nChannels           = num_channels;
        pcm.eNumData            = OMX_NumericalDataSigned;
        pcm.eEndian             = OMX_EndianLittle;
        pcm.nSamplingRate       = sample_rate;
        pcm.bInterleaved        = OMX_TRUE;
        pcm.nBitPerSample       = bit_depth;
        pcm.ePCMMode            = OMX_AUDIO_PCMModeLinear;

        switch(num_channels)
        {
            case 1:
            {
                pcm.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
                break;
            }
            case 8:
            {
                pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
                pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
                pcm.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
                pcm.eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
                pcm.eChannelMapping[4] = OMX_AUDIO_ChannelLR;
                pcm.eChannelMapping[5] = OMX_AUDIO_ChannelRR;
                pcm.eChannelMapping[6] = OMX_AUDIO_ChannelLS;
                pcm.eChannelMapping[7] = OMX_AUDIO_ChannelRS;
                break;
            }
            case 4:
            {
                pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
                pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
                pcm.eChannelMapping[2] = OMX_AUDIO_ChannelLR;
                pcm.eChannelMapping[3] = OMX_AUDIO_ChannelRR;
                break;
            }
            case 2:
            {
                pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
                pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
                break;
            }
        }

        error = OMX_SetParameter(ILC_GET_HANDLE(st->audio_render), OMX_IndexParamAudioPcm, &pcm);
        assert(error == OMX_ErrorNone);

        // ------------------
        // Set audio render reference clock.
        // ------------------

        OMX_CONFIG_BOOLEANTYPE  ref_clock;

        memset(&ref_clock, 0, sizeof(OMX_CONFIG_BOOLEANTYPE));
        param.nSize             = sizeof(OMX_CONFIG_BOOLEANTYPE);
        param.nVersion.nVersion = OMX_VERSION;
        param.bEnabled          = OMX_FALSE;

        error = OMX_SetConfig(ILC_GET_HANDLE(st->audio_render),
                              OMX_IndexConfigBrcmClockReferenceSource,
                              &ref_clock);
        assert(error == OMX_ErrorNone);

        // ------------------
        // Enable audio render.
        // ------------------

        ilclient_change_component_state(st->audio_render, OMX_StateIdle);
        if(ilclient_enable_port_buffers(st->audio_render, 100, NULL, NULL, NULL) < 0)
        {
            // error
            ilclient_change_component_state(st->audio_render, OMX_StateLoaded);
            ilclient_cleanup_components(st->list);

            error = OMX_Deinit();
            assert(error == OMX_ErrorNone);

            ilclient_destroy(st->client);

            sem_destroy(&st->sema);
            free(st);
            *handle = NULL;
            return -1;
        }

        ilclient_change_component_state(st->audio_render, OMX_StateExecuting);
    }

    return ret;
}


INT32 audioplay_set_dest(
    AUDIOPLAY_STATE_T  *st,
    const char         *name
    )
{
   INT32   success = -1;
   OMX_CONFIG_BRCMAUDIODESTINATIONTYPE ar_dest;

   if (name && strlen(name) < sizeof(ar_dest.sName))
   {
      OMX_ERRORTYPE error;
      memset(&ar_dest, 0, sizeof(ar_dest));
      ar_dest.nSize = sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE);
      ar_dest.nVersion.nVersion = OMX_VERSION;
      strcpy((char *)ar_dest.sName, name);

      error = OMX_SetConfig(ILC_GET_HANDLE(st->audio_render), OMX_IndexConfigBrcmAudioDestination, &ar_dest);
      assert(error == OMX_ErrorNone);
      success = 0;
   }

   return success;
}


UINT8   *audioplay_get_buffer(
    AUDIOPLAY_STATE_T *st
    )
{
   OMX_BUFFERHEADERTYPE *hdr = NULL;

   hdr = ilclient_get_input_buffer(st->audio_render, 100, 0);

   if(hdr)
   {
      // put on the user list
      sem_wait(&st->sema);

      hdr->pAppPrivate = st->user_buffer_list;
      st->user_buffer_list = hdr;

      sem_post(&st->sema);
   }

   return hdr ? hdr->pBuffer : NULL;
}


INT32 audioplay_play_buffer(
    AUDIOPLAY_STATE_T *st,
    UINT8   *buffer,
    UINT32   length
    )
{
   OMX_BUFFERHEADERTYPE *hdr = NULL, *prev = NULL;
   INT32 ret = -1;

   if(length % st->bytes_per_sample)
      return ret;

   sem_wait(&st->sema);

   // search through user list for the right buffer header
   hdr = st->user_buffer_list;
   while(hdr != NULL && hdr->pBuffer != buffer && hdr->nAllocLen < length)
   {
      prev = hdr;
      hdr = hdr->pAppPrivate;
   }

   if(hdr) // we found it, remove from list
   {
      ret = 0;
      if(prev)
         prev->pAppPrivate = hdr->pAppPrivate;
      else
         st->user_buffer_list = hdr->pAppPrivate;
   }

   sem_post(&st->sema);

   if(hdr)
   {
      OMX_ERRORTYPE error;

      hdr->pAppPrivate = NULL;
      hdr->nOffset = 0;
      hdr->nFilledLen = length;

      error = OMX_EmptyThisBuffer(ILC_GET_HANDLE(st->audio_render), hdr);
      assert(error == OMX_ErrorNone);
   }

   return ret;
}


UINT32  audioplay_get_latency(
    AUDIOPLAY_STATE_T *st
    )
{
   OMX_PARAM_U32TYPE    param;
   OMX_ERRORTYPE        error;


   memset(&param, 0, sizeof(OMX_PARAM_U32TYPE));
   param.nSize = sizeof(OMX_PARAM_U32TYPE);
   param.nVersion.nVersion = OMX_VERSION;
   param.nPortIndex = 100;

   error = OMX_GetConfig(ILC_GET_HANDLE(st->audio_render), OMX_IndexConfigAudioRenderingLatency, &param);
   assert(error == OMX_ErrorNone);

   return param.nU32;
}


#define BUFFER_SIZE (AUDIO_CODEC_FRAME_SIZE * (AUDIO_CODEC_BIT_DEPTH >> 3) * AUDIO_CODEC_NUM_CHANNEL)

// --------------------------------------------------------
// audio_decoder_hw_init
//      Initializes decoder hardware. 
//
void sx_audio_sink_init(
    void 
    )
{
    int rv; 


    // Initialize Broadcom host. 
    bcm_host_init();

    // Create audio object. 
    rv = audioplay_create(&st,
                          AUDIO_CODEC_SAMPLE_RATE,
                          AUDIO_CODEC_NUM_CHANNEL,
                          AUDIO_CODEC_BIT_DEPTH,
                          100, // TODO?
                          BUFFER_SIZE);
    assert(rv == 0); 

    rv = audioplay_set_dest(st, AUDIO_RENDERER); 
    assert(rv == 0); 

#if WRITE_TO_FILE
    write_ptr = fopen("audio_sink.raw", "wb");
#endif

    printf("(sx_audio_sink_init): Initialization completed...\n");
}


// --------------------------------------------------------
// decoder_hw_get_buf
//      Get decoder hardware buffer. 
//
UINT8 * sx_audio_sink_buffer_get(
    void
    )
{
    unsigned char  *buf = audioplay_get_buffer(st);
    assert(buf != NULL);

    // Type case to our overlay and return.
    return buf;
}


void sx_audio_sink_playback_speed_inc(
    )
{
    OMX_TIME_CONFIG_SCALETYPE  scale;
   OMX_ERRORTYPE error;

    memset(&scale, 0, sizeof(OMX_TIME_CONFIG_SCALETYPE));
    scale.nSize             = sizeof(OMX_TIME_CONFIG_SCALETYPE);
    scale.nVersion.nVersion = OMX_VERSION;
    scale.xScale            = 66000;

    error = OMX_SetConfig(ILC_GET_HANDLE(st->clock),
                          OMX_IndexConfigTimeScale,
                          &scale);
    assert(error == OMX_ErrorNone);
}


void sx_audio_sink_playback_speed_reset(
    )
{
    OMX_TIME_CONFIG_SCALETYPE  scale;
    OMX_ERRORTYPE error;

    memset(&scale, 0, sizeof(OMX_TIME_CONFIG_SCALETYPE));
    scale.nSize             = sizeof(OMX_TIME_CONFIG_SCALETYPE);
    scale.nVersion.nVersion = OMX_VERSION;
    scale.xScale            = 65536;

    error = OMX_SetConfig(ILC_GET_HANDLE(st->clock),
                          OMX_IndexConfigTimeScale,
                          &scale);
    assert(error == OMX_ErrorNone);
}


void sx_audio_sink_playback_speed_dec(
    )
{
    OMX_TIME_CONFIG_SCALETYPE   scale;
    OMX_ERRORTYPE               error;


    memset(&scale, 0, sizeof(OMX_TIME_CONFIG_SCALETYPE));
    scale.nSize             = sizeof(OMX_TIME_CONFIG_SCALETYPE);
    scale.nVersion.nVersion = OMX_VERSION;
    scale.xScale            = 65072;

    error = OMX_SetConfig(ILC_GET_HANDLE(st->clock),
                          OMX_IndexConfigTimeScale,
                          &scale);
    assert(error == OMX_ErrorNone);
}


// --------------------------------------------------------
// sx_audio_sink_ms_left_get
//      Get milliseconds left
//
unsigned int sx_audio_sink_ms_left_get(
    void
    )
{
    unsigned int samples_left = audioplay_get_latency(st);

    unsigned int ms_left = (samples_left/2)*1000/48000;

    return ms_left;
}


// --------------------------------------------------------
// sx_audio_sink_buffer_set
//      Set decoder hardware buffer. 
//
void sx_audio_sink_buffer_set(
    unsigned char  *buf, 
    unsigned int    buf_len
    )
{
    int             rv;
    OMX_ERRORTYPE   error;


#if WRITE_TO_FILE
    fwrite(buf, buf_len, 1, write_ptr);
#endif

    rv = audioplay_play_buffer(st, buf, buf_len);
    assert(rv == 0);
}
