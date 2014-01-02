#include <string.h>
#include <pthread.h>

#include <assert.h>

void sx_thread_create(
    void           *thread_id,
    void           *thread_func,
    void           *thread_arg,
    unsigned int    thread_priority
    )
{
    struct sched_param param;


    // Create decoder thread.
    pthread_create(thread_id, NULL, (void *) thread_func, NULL);

    // Set thread priority.
    // Launch the program using sudo to honor the priority setting!
    memset(&param, 0, sizeof(param));
    param.sched_priority = thread_priority;
    pthread_setschedparam(*((int *) thread_id), SCHED_FIFO, &param);
}
