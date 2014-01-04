#include <stdlib.h>
#include <assert.h>

#include "sx_desc.h"

// --------------------------------------------------------
// sx_desc_get
//      Gets a descriptor
//
sSX_DESC * sx_desc_get(
    void
    )
{
    sSX_DESC *desc = malloc(sizeof(sSX_DESC));
    assert(desc != NULL);

    desc->data      = NULL;
    desc->data_len  = 0;
    desc->next      = NULL;

    return desc;
}


// --------------------------------------------------------
// sx_desc_put
//      Frees a descriptor (or a descriptor chain)
//
void sx_desc_put(
     sSX_DESC  *desc
     )
{
     sSX_DESC  *curr;
     sSX_DESC  *next;


     // Consistency check.
     assert(desc != NULL);

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
