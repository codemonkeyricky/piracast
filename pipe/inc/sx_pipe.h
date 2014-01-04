#if !defined(_SX_VRDMA_H_)
#define _SX_VRDMA_H_

#define SX_VRDMA_PKT_QUEUE          0
#define SX_VRDMA_VIDEO_PKT_QUEUE    1
#define SX_VRDMA_SLICE          2
#define SX_VRDMA_SLICE_READY    3
#define SX_VRDMA_PCR            4
#define SX_VRDMA_LPCM           5
#define SX_VRDMA_LPCM_SLICE     6
#define SX_VRDMA_MAX            7

extern void sx_pipe_init(
    void
    );

extern void sx_pipe_put(
    UINT32  index,
    void   *data
    );

extern void * sx_pipe_get(
    UINT32  index
    );

extern unsigned int sx_pipe_len_get(
    UINT32  index
    );

#endif // _SX_VRDMA_H_
