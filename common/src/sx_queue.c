#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <stdio.h>

#include "sx_types.h"
#include "sx_queue.h"

typedef struct sNODE
{
    struct sNODE   *next;
    void           *data;

} sNODE;


typedef struct
{
    sNODE          *head;
    sNODE          *tail;
    pthread_mutex_t lock;
    sem_t           sem;
    unsigned int    len;
    UINT32          high_water_mark;

} sQUEUE;


// --------------------------------------------------------
// rt_queue_create
//      Create a queue.
//
SX_QUEUE sx_queue_create(
    void
    )
{
    sQUEUE *queue = malloc(sizeof(sQUEUE));

    pthread_mutex_init(&queue->lock, NULL);

    sem_init(&queue->sem, 0, 0);

    queue->head = NULL;
    queue->tail = NULL;
    queue->len  = 0;

    return queue;
}


// --------------------------------------------------------
// rt_queue_destroy
//      Destory a queue.
//
void sx_queue_destroy(
    SX_QUEUE    queue_id
    )
{
    sQUEUE *queue;
    sNODE  *curr;
    sNODE  *next;


    // Get the queue.
    queue = queue_id;

    // Free every node.
    curr = queue->head;
    while(curr)
    {
        next = curr->next;

        // Free data.
        free(curr->data);

        // Free node.
        free(curr);

        curr = next;
    }

    // Destroy mutex.
    pthread_mutex_destroy(&queue->lock);

    // Free queue.
    free(queue);
}


// --------------------------------------------------------
// rt_queue_push
//      Push a data payload.
//
void sx_queue_push(
    SX_QUEUE    queue_id,
    void       *data
    )
{
    sNODE  *node;
    sQUEUE *queue;


    assert(data != NULL);

    // Get queue.
    queue = queue_id;

    // Construct new node.
    node = malloc(sizeof(sNODE));
    node->data = data;
    node->next = NULL;

    // Lock.
    pthread_mutex_lock(&queue->lock);

#if 0
    printf("(rt_queue): push(): id = 0x%x, len = %d\n",
            (int) queue_id,
            queue->len);
#endif

    // Is list empty?
    if(queue->head == NULL)
    {
        // List is empty, insert only node.
        queue->head = node;

        queue->tail = node;

        goto cleanup;
    }

    // Append to end.
    queue->tail->next = node;

    // Update tail.
    queue->tail = node;

cleanup:

    queue->len++;

//    if(queue->len > queue->high_water_mark)
//    {
//        printf("(rt_queue): push(): id = 0x%x, len = %d\n",
//               queue_id,
//               queue->len);
//
//        queue->high_water_mark = queue->len;
//    }

    pthread_mutex_unlock(&queue->lock);

//    sem_post(&queue->sem);
}


// --------------------------------------------------------
// rt_queue_pull
//      Pull a data payload.
//
void * sx_queue_pull(
    SX_QUEUE    queue_id
    )
{
    sQUEUE         *queue;
    unsigned char  *data;


    // Get queue.
    queue = queue_id;

//    sem_wait(&queue->sem);

    // Lock.
    pthread_mutex_lock(&queue->lock);

#if 0
    printf("(rt_queue): pull(): id = 0x%x, len = %d\n",
            (int) queue,
            queue->len);
#endif

    // Is list empty?
    if(queue->head == NULL)
    {
        assert(queue->tail == NULL);

        // List is empty.
        data = NULL;

        goto cleanup;
    }

    if(queue->head == queue->tail)
    {
        assert(queue->head->next == NULL);
        assert(queue->tail->next == NULL);

        // List only has one element.
        data = queue->head->data;

        // Free the node.
        free(queue->head);

        // Reset head and tail.
        queue->head = NULL;
        queue->tail = NULL;

        goto cleanup;
    }

    // More than one element.

    // Return packet.
    data = queue->head->data;

    // Get next.
    sNODE *next = queue->head->next;

    // Free head.
    free(queue->head);

    // Advance head.
    queue->head = next;

cleanup:

    if(data != NULL)
    {
        queue->len--;
    }

    pthread_mutex_unlock(&queue->lock);

    return data;
}


unsigned int sx_queue_len_get(
    SX_QUEUE    queue_id
    )
{
    sQUEUE         *queue;
    unsigned int    len;


    // Get queue.
    queue = queue_id;

    pthread_mutex_lock(&queue->lock);

    len = queue->len;

    pthread_mutex_unlock(&queue->lock);

    return len;
}
