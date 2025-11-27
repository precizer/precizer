#include "mem.h"
#include <string.h>

Return memory_copy(
	memory       *destination,
	const memory *source)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(destination == NULL || source == NULL ||
	        destination->element_size == 0 || source->element_size == 0)
	{
		slog(ERROR,"Memory management; At least one memory structure remains uninitialized");
		provide(FAILURE);
	}

	if(destination->element_size != source->element_size)
	{
		slog(ERROR,"Memory management; Element size mismatch (%zu vs %zu)",
			destination->element_size,
			source->element_size);
		provide(FAILURE);
	}

	size_t bytes_to_copy = 0;

	run(memory_guarded_size(source->element_size,source->length,&bytes_to_copy));

	if(FAILURE == status)
	{
		slog(ERROR,"Memory management; Overflow computing %zu * %zu",
			source->element_size,
			source->length);
	}

	if(SUCCESS == status && destination->length != source->length)
	{
		run(resize(destination,source->length));
	}

	if(SUCCESS == status && source->length > 0)
	{
		memcpy(destination->data,source->data,bytes_to_copy);
	}

	provide(status);
}
