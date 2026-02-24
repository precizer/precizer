#include "mem.h"
#include <string.h>

Return memory_copy(
	memory       *destination,
	const memory *source)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
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
		report("Memory management; Element size mismatch (%zu vs %zu)",
			destination->element_size,
			source->element_size);
		provide(FAILURE);
	}

	size_t bytes_to_copy = 0;

	run(memory_guarded_size(source->element_size,source->length,&bytes_to_copy));

	if(CRITICAL & status)
	{
		report("Memory management; Overflow computing %zu * %zu",
			source->element_size,
			source->length);
	}

	if((TRIUMPH & status) && destination->length != source->length)
	{
		run(resize(destination,source->length));
	}

	if((TRIUMPH & status) && source->length > 0)
	{
		memcpy(destination->data,source->data,bytes_to_copy);
	}

	provide(status);
}
