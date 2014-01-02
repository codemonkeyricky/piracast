#if !defined(MGMT_SYS_H)
#define MGMT_SYS_H

typedef enum
{
    MGMT_SYS_TYPE_ENCODER, 
    MGMT_SYS_TYPE_DECODER

} eMGMT_SYS_TYPE; 

extern void mgmt_sys_open(
    void 
    );

#endif
