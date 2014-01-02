#if !defined(AUDIO_SINK_H)
#define AUDIO_SINK_H


extern void sx_audio_sink_init(
    void 
);

extern UINT8 * sx_audio_sink_buffer_get(
    void
    ); 

extern unsigned int sx_audio_sink_ms_left_get(
    void
    );

extern void sx_audio_sink_buffer_set(
    unsigned char  *buf, 
    unsigned int    buf_len
    ); 

extern void sx_audio_sink_playback_speed_inc(
    void
    );

extern void sx_audio_sink_playback_speed_reset(
    void
    );

extern void sx_audio_sink_playback_speed_dec(
    void
    );

#endif
