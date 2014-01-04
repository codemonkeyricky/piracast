#include "sx_types.h"
#include "sx_queue.h"
#include "sx_pipe.h"


typedef struct
{
    SX_QUEUE    queue[SX_VRDMA_MAX];

} sVRDMA_CBLK;


static sVRDMA_CBLK f_cblk;


void sx_pipe_init(
    void
    )
{
    UINT32  i;


    for(i = 0; i < SX_VRDMA_MAX; i++)
    {
        // Create queue.
        f_cblk.queue[i] = sx_queue_create();
    }
}


void sx_pipe_put(
    UINT32  index,
    void   *data
    )
{
    sx_queue_push(f_cblk.queue[index], data);
}


void * sx_pipe_get(
    UINT32  index
    )
{
    return sx_queue_pull(f_cblk.queue[index]);
}


unsigned int sx_pipe_len_get(
    UINT32  index
    )
{
    return sx_queue_len_get(f_cblk.queue[index]);
}
