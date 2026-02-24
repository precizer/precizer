#include "mem.h"
#include <string.h>

Return memory_append(
	memory       *destination,
	const memory *source)
{
	Return status = SUCCESS;

	if(destination == NULL || source == NULL)
	{
		report("Memory management; append arguments must be non-NULL");
		provide(FAILURE);
	}

	if(destination->element_size == 0)
	{
		report("Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source->element_size == 0)
	{
		report("Memory management; Source element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(destination->element_size != source->element_size)
	{
		report("Memory management; Element size mismatch (%zu vs %zu)",destination->element_size,source->element_size);
		provide(FAILURE);
	}

	const size_t append_elements = source->length;

	if(append_elements == 0)
	{
		provide(status);
	}

	if(append_elements > SIZE_MAX - destination->length)
	{
		report("Memory management; Append would overflow element count");
		provide(FAILURE);
	}

	size_t append_bytes = 0;

	run(memory_guarded_size(source->element_size,append_elements,&append_bytes));

	if(CRITICAL & status)
	{
		report("Memory management; Overflow computing append bytes");
	}

	size_t offset_bytes = 0;

	run(memory_guarded_size(destination->element_size,destination->length,&offset_bytes));

	if(CRITICAL & status)
	{
		report("Memory management; Overflow computing append offset");
	}

	size_t new_total_elements;

	if(TRIUMPH & status)
	{
		new_total_elements = destination->length + append_elements;
	}

	run(resize(destination,new_total_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL || source->data == NULL)
		{
			report("Memory management; Data pointer is NULL during append");
			status = FAILURE;
		} else {
			memcpy(destination_bytes + offset_bytes,source->data,append_bytes);
		}
	}

	provide(status);
}
