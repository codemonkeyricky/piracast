#if !defined(_MGMT_ENV_H_)
#define _MGMT_ENV_H_

#include "sx_types.h"

typedef enum
{
    MGMT_ENV_VAR_WIDTH,
    MGMT_ENV_VAR_HEIGHT,
    MGMT_ENV_VAR_SESSION_WIDTH,
    MGMT_ENV_VAR_SESSION_HEIGHT,

    MGMT_ENV_VAR_MAX

} eMGMT_ENV_VAR;

extern void sx_mgmt_env_init(
    void
    );

extern void sx_mgmt_env_open(
    void
    );

extern UINT32 sx_mgmt_env_get(
    eMGMT_ENV_VAR   var
    );

#endif
