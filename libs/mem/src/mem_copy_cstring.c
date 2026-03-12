#include "mem.h"
#include <string.h>

/**
 * @brief Copy visible bytes from a bounded source string buffer.
 *
 * Visible source length is measured with @ref memory_string_length semantics
 * over a temporary descriptor view created from the provided source range.
 */
Return memory_copy_cstring(
	memory     *destination,
	const char *source_buffer,
	size_t     source_buffer_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	create(char,source_view);
	size_t source_length = 0;
	size_t total_elements = 0;

	if(destination == NULL)
	{
		report("Memory management; copy_cstring destination must be non-NULL");
		status = FAILURE;
	}

	if((TRIUMPH & status) && destination->element_size != sizeof(char))
	{
		report("Memory management; copy_cstring supports byte-sized elements only");
		status = FAILURE;
	}

	if((TRIUMPH & status) && source_buffer == NULL && source_buffer_size > 0)
	{
		report("Memory management; copy_cstring source is NULL while size is %zu",source_buffer_size);
		status = FAILURE;
	}

	run(copy_buffer(source_view,source_buffer,source_buffer_size));
	run(string_length(source_view,&source_length));

	if(TRIUMPH & status)
	{
		if(source_length == SIZE_MAX)
		{
			report("Memory management; Not enough room for string terminator");
			status = FAILURE;
		} else {
			total_elements = source_length + 1;
		}
	}

	run(resize(destination,total_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL)
		{
			report("Memory management; Destination data pointer is NULL after resize");
			status = FAILURE;
		} else {
			if(source_length > 0)
			{
				memcpy(destination_bytes,source_buffer,source_length);
			}

			destination_bytes[source_length] = '\0';
			telemetry_string_padding_event();
		}
	}

	call(del(source_view));

	provide(status);
}
