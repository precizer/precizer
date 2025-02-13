#include "mem.h"

void FREE_AND_RESET(void **ptr)
{
    if(NULL != ptr)
    {
        free(*ptr);
        *ptr = NULL;
    }
}
