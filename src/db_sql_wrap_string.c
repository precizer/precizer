#include "precizer.h"

/**
 * @brief Wrap a plain string into an SQL literal with escaping.
 *
 * @param destination Pointer to a byte-sized memory descriptor receiving the wrapped string.
 * @param source      Zero-terminated string that should be quoted for SQL usage.
 *
 * @return Return code describing operation status.
 */
Return db_sql_wrap_string(
	memory     *destination,
	const char *source)
{
	/** Return status
	 *  The status that will be passed to return() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	if(destination == NULL || source == NULL)
	{
		slog(ERROR,"sql_wrap_string arguments must be non-NULL\n");
		status = FAILURE;
	}

	if(SUCCESS == status && destination->element_size != sizeof(char))
	{
		slog(ERROR,"sql_wrap_string requires byte-sized destination descriptor\n");
		status = FAILURE;
	}

	size_t source_length = 0;

	if(SUCCESS == status)
	{
		source_length = strlen(source);
	}

	size_t apostrophes = 0;

	if(SUCCESS == status)
	{
		for(size_t i = 0; i < source_length; ++i)
		{
			if(source[i] == '\'')
			{
				if(apostrophes == SIZE_MAX)
				{
					slog(ERROR,"sql_wrap_string apostrophe counter overflow\n");
					status = FAILURE;
					break;
				}

				++apostrophes;
			}
		}
	}

	size_t required_elements = 0;

	if(SUCCESS == status)
	{
		const size_t overhead = 3;

		if(source_length > SIZE_MAX - apostrophes)
		{
			slog(ERROR,"sql_wrap_string overflow while adding escape budget\n");
			status = FAILURE;
		} else {
			size_t base_length = source_length + apostrophes;

			if(base_length > SIZE_MAX - overhead)
			{
				slog(ERROR,"sql_wrap_string overflow before allocating terminator\n");
				status = FAILURE;
			} else {
				required_elements = base_length + overhead;
			}
		}
	}

	run(resize(destination,required_elements));

	if(SUCCESS == status)
	{
		unsigned char *destination_bytes = data(unsigned char,destination);

		if(destination_bytes == NULL)
		{
			slog(ERROR,"sql_wrap_string destination pointer is NULL after resize\n");
			status = FAILURE;
		} else {
			size_t write_index = 0;

			destination_bytes[write_index++] = '\'';

			for(size_t i = 0; i < source_length; ++i)
			{
				unsigned char current_char = (unsigned char)source[i];

				if(current_char == '\'')
				{
					/* Double apostrophes for SQL-compatible escaping */
					destination_bytes[write_index++] = '\'';
					destination_bytes[write_index++] = '\'';
				} else {
					destination_bytes[write_index++] = current_char;
				}
			}

			destination_bytes[write_index++] = '\'';
			destination_bytes[write_index] = '\0';
			telemetry_string_padding_event();
		}
	}

	return(status);
}
