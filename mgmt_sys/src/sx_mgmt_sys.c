#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>

#include "sx_thread_priority.h"
#include "sx_mgmt_sys.h"
#include "sx_mgmt_m2ts_decoder.h"
#include "sx_mgmt_video_decoder.h"
#include "sx_mgmt_data.h"
#include "sx_mgmt_ctl.h"


static pthread_t f_mgmt_sys_thread;

static void resource_init(
    void
    )
{
#if 0
    struct mq_attr queue_attr;

    // Initialize queue attributes.
    queue_attr.mq_flags     = 0;
    queue_attr.mq_maxmsg    = 10;
    queue_attr.mq_msgsize   = sizeof(sMGMT_SYS_MSG); 
    queue_attr.mq_curmsgs   = 0;

    // Create message queue.
    f_cblk.mgmt_sys_msg_queue = mq_open(MGMT_SYS_MSG_QUEUE_NAME, O_CREAT | O_RDWR, 0644, &queue_attr);
    assert(f_cblk.mgmt_sys_msg_queue > 0);
#endif
}


static void wait_forever(
    void
    )
{
    pthread_join(f_mgmt_sys_thread, NULL);
}


void mgmt_sys_init(
    void
    )
{
    // Initialize system manager. 
    resource_init(); 

    // Initialize VRDMA.
    sx_pipe_init();

//    sx_mgmt_env_init();

    // Initialize Data manager. 
    sx_mgmt_data_init(); 

    // Initialize m2ts decoder.
    sx_mgmt_m2ts_decoder_init();

    // Initialize video decoder module.
    sx_mgmt_video_decoder_init();

    sx_mgmt_video_scheduler_init();

    // Initialize video decoder module.
    sx_mgmt_audio_decoder_init();

    sx_mgmt_audio_scheduler_init();

    printf("(mgmt_sys): (mgmt_sys_init): Done.\n"); 
}


static void mgmt_sys_thread(
    void * arg
    )
{
    do
    {
        usleep(1*1000*1000);

    } while(1);
}


void mgmt_sys_open(
    void
    )
{
    // Initialize Data manager.
    sx_mgmt_data_open();

    // Initialize m2ts decoder.
    sx_mgmt_m2ts_decoder_open();

    // Initialize video decoder module.
    sx_mgmt_video_decoder_open();

    sx_mgmt_video_scheduler_open();

    // Initialize video decoder module.
    sx_mgmt_audio_decoder_open();

    sx_mgmt_audio_scheduler_open();

    printf("(mgmt_sys): (mgmt_sys_open): Done.\n");

    sx_thread_create(&f_mgmt_sys_thread, &mgmt_sys_thread, NULL, MGMT_SYS_THREAD_PRIORITY);

    // Wait forever.
    wait_forever();
   // sx_mgmt_ctl_wait();
}
