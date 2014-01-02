#include <assert.h>

#include "sx_mgmt_sys.h"


int main(
    int     argc,
    char   *argv[]
    )
{
    // Initialized system.
    mgmt_sys_init();

    // Open system.
    mgmt_sys_open();
}
