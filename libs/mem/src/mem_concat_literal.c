#include "mem.h"
#include <string.h>

Return memory_concat_literal(
	memory     *destination,
	const char *literal)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		report("Memory management; concat_literal destination must be non-NULL");
		status = FAILURE;
	}

	if((TRIUMPH & status) && literal == NULL)
	{
		/* Treat NULL literals as a no-op to keep destination intact */
		provide(status);
	}

	if((TRIUMPH & status) && destination->element_size != sizeof(char))
	{
		report("Memory management; concat_literal supports byte-sized elements only");
		status = FAILURE;
	}

	size_t destination_length = 0;

	run(string_length(destination,&destination_length));

	size_t literal_length = 0;

	if(TRIUMPH & status)
	{
		literal_length = strlen(literal);
	}

	size_t new_total_elements = 0;

	if(TRIUMPH & status)
	{
		if(destination_length > SIZE_MAX - literal_length)
		{
			report("Memory management; Concatenating literal would overflow element count");
			status = FAILURE;
		} else {
			const size_t sum = destination_length + literal_length;

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

	run(memory_guarded_size(destination->element_size,destination_length,&offset_bytes));

	size_t literal_bytes = 0;

	run(memory_guarded_size(destination->element_size,literal_length,&literal_bytes));

	run(resize(destination,new_total_elements));

	if(TRIUMPH & status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL)
		{
			report("Memory management; Destination data pointer is NULL after resize");
			status = FAILURE;
		} else {
			if(literal_bytes > 0)
			{
				memmove(destination_bytes + offset_bytes,literal,literal_bytes);
			}

			destination_bytes[offset_bytes + literal_bytes] = '\0';
			telemetry_string_padding_event();
		}
	}

	provide(status);
}
