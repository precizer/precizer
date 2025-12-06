#include "mem.h"
#include <string.h>

Return memory_append(
	memory       *destination,
	const memory *source)
{
	Return status = SUCCESS;

	if(destination == NULL || source == NULL)
	{
		slog(ERROR,"Memory management; append arguments must be non-NULL");
		provide(FAILURE);
	}

	if(destination->element_size == 0)
	{
		slog(ERROR,"Memory management; Destination element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(source->element_size == 0)
	{
		slog(ERROR,"Memory management; Source element size is zero (uninitialized)");
		provide(FAILURE);
	}

	if(destination->element_size != source->element_size)
	{
		slog(ERROR,
			"Memory management; Element size mismatch (%zu vs %zu)",
			destination->element_size,
			source->element_size);
		provide(FAILURE);
	}

	const size_t append_elements = source->length;

	if(append_elements == 0)
	{
		provide(status);
	}

	const size_t original_elements = destination->length;

	if(append_elements > SIZE_MAX - original_elements)
	{
		slog(ERROR,"Memory management; Append would overflow element count");
		provide(FAILURE);
	}

	size_t append_bytes = 0;

	run(memory_guarded_size(source->element_size,append_elements,&append_bytes));

	if(FAILURE == status)
	{
		slog(ERROR,"Memory management; Overflow computing append bytes");
	}

	size_t offset_bytes = 0;

	run(memory_guarded_size(destination->element_size,original_elements,&offset_bytes));

	if(FAILURE == status)
	{
		slog(ERROR,"Memory management; Overflow computing append offset");
	}

	const size_t new_total_elements = original_elements + append_elements;

	if(SUCCESS == status)
	{
		run(resize(destination,new_total_elements));
	}

	if(SUCCESS == status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL || source->data == NULL)
		{
			slog(ERROR,"Memory management; Data pointer is NULL during append");
			status = FAILURE;
		} else {
			memcpy(destination_bytes + offset_bytes,source->data,append_bytes);
		}
	}

	provide(status);
}
