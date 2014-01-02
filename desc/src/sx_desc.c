#include <stdlib.h>
#include "sx_desc.h"

sSX_DESC * sx_desc_get(
    void
    )
{
    sSX_DESC *desc = malloc(sizeof(sSX_DESC));

    desc->data      = NULL;
    desc->data_len  = 0;
    desc->next      = NULL;

    return desc;
}


void sx_desc_put(
     sSX_DESC  *desc
     )
{
     sSX_DESC  *curr;
     sSX_DESC  *next;


     // Initialize current.
     curr = desc;

     // Release descriptors and data.
     while(curr != NULL)
     {
         next = curr->next;

         free(curr->data);
         free(curr);

         curr = next;
     }
}
