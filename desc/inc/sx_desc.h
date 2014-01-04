#if !defined(_SX_DESC_H_)
#define _SX_DESC_H_

#include "sx_types.h"

typedef struct sSX_DESC
{
    UINT8              *data;
    UINT32              data_len;
    struct sSX_DESC    *next;

} sSX_DESC;

extern sSX_DESC * sx_desc_get(
    void
    );

extern void sx_desc_put(
     sSX_DESC  *desc
     );

#endif // _SX_DESC_H_
