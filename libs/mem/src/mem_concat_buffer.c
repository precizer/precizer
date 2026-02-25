#include "mem.h"

/**
 * @brief Concatenate a raw byte buffer to a descriptor.
 *
 * Interprets @p source_buffer_size as the exact number of bytes to append.
 * The destination is resized by `source_buffer_size / element_size` elements
 * and receives a byte-for-byte payload copy.
 *
 * This helper performs binary concatenation only. It does not scan for
 * `'\0'`, does not enforce string termination, and preserves embedded zero
 * bytes exactly as provided.
 */
Return memory_concat_buffer(
	memory     *destination,
	const void *source_buffer,
	size_t     source_buffer_size)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		report("Memory management; concat_buffer destination must be non-NULL");
		provide(FAILURE);
	}

	if(destination->element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source_buffer == NULL)
	{
		if(source_buffer_size == 0)
		{
			provide(status);
		}

		report("Memory management; concat_buffer source is NULL while size is %zu",source_buffer_size);
		provide(FAILURE);
	}

	if((source_buffer_size % destination->element_size) != 0)
	{
		report("Memory management; concat_buffer size %zu is not divisible by element size %zu",source_buffer_size,destination->element_size);
		provide(FAILURE);
	}

	size_t append_elements = 0;

	if(TRIUMPH & status)
	{
		append_elements = source_buffer_size / destination->element_size;
	}

	if(append_elements == 0)
	{
		provide(status);
	}

	if(append_elements > SIZE_MAX - destination->length)
	{
		report("Memory management; concat_buffer would overflow element count");
		provide(FAILURE);
	}

	size_t offset_bytes = 0;

	run(memory_guarded_size(destination->element_size,destination->length,&offset_bytes));

	if(CRITICAL & status)
	{
		report("Memory management; Overflow computing concat_buffer destination offset");
		provide(status);
	}

	run(resize(destination,destination->length + append_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL)
		{
			report("Memory management; Destination data pointer is NULL during concat_buffer");
			status = FAILURE;
		} else {
			memcpy(destination_bytes + offset_bytes,source_buffer,source_buffer_size);
		}
	}

	provide(status);
}
