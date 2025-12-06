#include "mem.h"
#include <string.h>

Return memory_copy_literal(
	memory     *destination,
	const char *literal)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(destination == NULL)
	{
		slog(ERROR,"Memory management; copy_literal destination must be non-NULL");
		status = FAILURE;
	}

	if(SUCCESS == status && literal == NULL)
	{
		/* Treat NULL literals as a no-op to keep existing payload intact */
		provide(SUCCESS);
	}

	if(SUCCESS == status && destination->element_size != sizeof(char))
	{
		slog(ERROR,"Memory management; copy_literal supports byte-sized elements only");
		status = FAILURE;
	}

	size_t literal_length = 0;

	if(SUCCESS == status)
	{
		literal_length = strlen(literal);
	}

	size_t new_total_elements = 0;

	if(SUCCESS == status)
	{
		if(literal_length == SIZE_MAX)
		{
			slog(ERROR,"Memory management; Not enough room for string terminator");
			status = FAILURE;
		} else {
			new_total_elements = literal_length + 1;
		}
	}

	size_t literal_bytes = 0;

	run(memory_guarded_size(destination->element_size,literal_length,&literal_bytes));

	run(resize(destination,new_total_elements));

	if(SUCCESS == status)
	{
		unsigned char *destination_bytes = (unsigned char *)destination->data;

		if(destination_bytes == NULL)
		{
			slog(ERROR,"Memory management; Destination data pointer is NULL after resize");
			status = FAILURE;
		} else {
			if(literal_bytes > 0)
			{
				memcpy(destination_bytes,literal,literal_bytes);
			}

			destination_bytes[literal_bytes] = '\0';
			telemetry_string_padding_event();
		}
	}

	provide(status);
}
