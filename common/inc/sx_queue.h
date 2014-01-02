#if !defined(_SX_QUEUE_H_)
#define _SX_QUEUE_H_

#define SX_QUEUE    void *

extern SX_QUEUE sx_queue_create(
    void
    );

extern void sx_queue_destroy(
    SX_QUEUE    queue
    );

extern void sx_queue_push(
    SX_QUEUE    queue,
    void       *data
    );

extern void * sx_queue_pull(
    SX_QUEUE    queue
    );

extern unsigned int sx_queue_len_get(
    SX_QUEUE    queue_id
    );

#endif // _SX_QUEUE_H_
