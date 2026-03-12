#include "mem.h"
#include <string.h>

Return memory_concat_strings(
	memory       *destination,
	const memory *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL || source == NULL)
	{
		report("Memory management; concat_strings arguments must be non-NULL");
		provide(FAILURE);
	}

	if(destination->element_size != source->element_size)
	{
		report("Memory management; Element size mismatch (%zu vs %zu)",destination->element_size,source->element_size);
		provide(FAILURE);
	}

	if(destination->element_size != sizeof(char))
	{
		report("Memory management; concat_strings supports byte-sized elements only");
		provide(FAILURE);
	}

	size_t destination_length = 0;
	size_t source_length = 0;

	run(string_length(destination,&destination_length));
	run(string_length(source,&source_length));

	size_t new_total_elements = 0;

	if(TRIUMPH & status)
	{
		if(destination_length > SIZE_MAX - source_length)
		{
			report("Memory management; Concatenation would overflow element count");
			status = FAILURE;
		} else {
			const size_t sum = destination_length + source_length;

			if(sum == SIZE_MAX)
			{
				report("Memory management; Not enough room for string terminator");
				status = FAILURE;
			} else {
				new_total_elements = sum + 1;
			}
		}
	}

	size_t offset_bytes = 0;
	size_t source_bytes = 0;

	run(memory_guarded_size(destination->element_size,destination_length,&offset_bytes));
	run(memory_guarded_size(source->element_size,source_length,&source_bytes));
	run(resize(destination,new_total_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL)
		{
			report("Memory management; Destination data pointer is NULL after resize");
			status = FAILURE;
		} else {
			if(source_bytes > 0)
			{
				const unsigned char *source_bytes_ptr = (const unsigned char *)source->data;

				if(source_bytes_ptr == NULL)
				{
					report("Memory management; Source data pointer is NULL");
					status = FAILURE;
				} else {
					memmove(destination_bytes + offset_bytes,source_bytes_ptr,source_bytes);
				}
			}

			if(TRIUMPH & status)
			{
				destination_bytes[offset_bytes + source_bytes] = '\0';
				telemetry_string_padding_event();
			}
		}
	}

	provide(status);
}
