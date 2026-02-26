#include "mem.h"
#include <string.h>

/**
 * @brief Import an externally-owned byte buffer into a descriptor.
 *
 * This helper mirrors @ref memory_copy semantics for size checks and resize
 * flow, but takes raw pointer + byte count instead of a source descriptor.
 */
Return memory_copy_buffer(
	memory     *destination,
	const void *source_buffer,
	size_t     buffer_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		report("Memory management; copy_buffer destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source_buffer == NULL)
	{
		if(buffer_size == 0)
		{
			run(resize(destination,0));
			provide(status);
		}

		report("Memory management; copy_buffer source is NULL while size is %zu",buffer_size);
		provide(FAILURE);
	}

	if((buffer_size % destination->element_size) != 0)
	{
		report("Memory management; copy_buffer size %zu is not divisible by element size %zu",buffer_size,destination->element_size);
		provide(FAILURE);
	}

	const size_t target_elements = buffer_size / destination->element_size;

	run(resize(destination,target_elements));

	if((TRIUMPH & status) && buffer_size > 0)
	{
		if(destination->data == NULL)
		{
			report("Memory management; Destination data pointer is NULL during copy_buffer");
			status = FAILURE;
		} else {
			memcpy(destination->data,source_buffer,buffer_size);
		}
	}

	provide(status);
}
