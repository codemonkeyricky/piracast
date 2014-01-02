#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "sx_types.h"
#include "sx_mgmt_env.h"

typedef struct
{
    eMGMT_ENV_VAR   var;
    char           *str;

} sMGMT_ENV_VAR_ENTRY;


static sMGMT_ENV_VAR_ENTRY f_env_var_table[] =
{
        { MGMT_ENV_VAR_WIDTH, "displayWidth" },
        { MGMT_ENV_VAR_HEIGHT, "displayHeight" },
        { MGMT_ENV_VAR_SESSION_WIDTH, "sessionWidth" },
        { MGMT_ENV_VAR_SESSION_HEIGHT, "sessionHeight" }
};
#define MGMT_ENV_VAR_TABLE_SIZE (sizeof(f_env_var_table)/sizeof(f_env_var_table[0]))


static UINT32 f_env_vars[MGMT_ENV_VAR_MAX] = {0};


void sx_mgmt_env_init(
    void
    )
{
    FILE * read_ptr = fopen("env.txt", "r");
    assert(read_ptr != NULL);

    char line[128];
    UINT32  i;
    UINT8   found;
    UINT32  value;

    while(fgets(line, sizeof(line), read_ptr) != NULL)
    {
        line[strlen(line) - 1] = 0;

        found = 0;

        for(i = 0; i < MGMT_ENV_VAR_TABLE_SIZE; i++)
        {
            if(strstr(line, f_env_var_table[i].str) != NULL)
            {
                printf("found!!\n");

                found = 1;

                break;
            }
        }

        if(found)
        {
//            char var_name[20];

//            printf("line= %s\n", line);

            sscanf(line, "%*s %d", &value);

            f_env_vars[f_env_var_table[i].var] = value;

            printf("value = %u\n", value);
        }
    }
}


void sx_mgmt_env_open(
    void
    )
{

}


UINT32 sx_mgmt_env_get(
    eMGMT_ENV_VAR   var
    )
{
    printf("(sx_mgmt_env_get(): var = %u\n", f_env_vars[var]);

    return f_env_vars[var];
}
