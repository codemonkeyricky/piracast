#if !defined(_RT_THREAD_H_)
#define _RT_THREAD_H_

#include "sx_thread_priority.h"

extern void sx_thread_create(
    void           *thread_id,
    void           *thread_func,
    void           *thread_arg,
    unsigned int    thread_priority
    );

#endif // #if !defined(_RT_THREAD_H_)
