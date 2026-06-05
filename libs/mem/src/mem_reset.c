#include "mem.h"

#ifdef reset
#undef reset
#endif

/**
 * @brief Free the allocation referenced by @p pointer_handle and null it out.
 *
 * Accepts a pointer to any dynamically allocated pointer variable, calls
 * @ref free when the variable is non-NULL, and guarantees @p *pointer_handle
 * becomes NULL afterwards. A NULL handle is ignored.
 *
 * @param pointer_handle Address of the pointer being released.
 */
void mem_free_and_reset(void **pointer_handle)
{
	if(pointer_handle != NULL)
	{
		free(*pointer_handle);
		*pointer_handle = NULL;
	}
}
