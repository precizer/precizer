#include "precizer.h"

/**
 * @brief Wrap a plain string into an SQL literal with escaping.
 *
 * @param destination Pointer to a byte-sized string descriptor receiving the wrapped string.
 * @param source      Zero-terminated string that should be quoted for SQL usage.
 *
 * @return Return code describing operation status.
 */
Return db_sql_wrap_string(
	memory     *destination,
	const char *source)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(destination == NULL || source == NULL)
	{
		slog(ERROR,"sql_wrap_string arguments must be non-NULL\n");
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

	size_t escaped_payload_length = 0;
	size_t escaped_visible_length = 0;
	size_t required_elements = 0;

	if(SUCCESS == status)
	{
		run(m_guarded_add(source_length,apostrophes,&escaped_payload_length));

		if(CRITICAL & status)
		{
			slog(ERROR,"sql_wrap_string overflow while adding escape budget\n");
		}
	}

	if(SUCCESS == status)
	{
		run(m_guarded_add(escaped_payload_length,2,&escaped_visible_length));

		if(CRITICAL & status)
		{
			slog(ERROR,"sql_wrap_string overflow while adding surrounding quotes\n");
		}
	}

	if(SUCCESS == status)
	{
		run(m_guarded_add(escaped_visible_length,1,&required_elements));

		if(CRITICAL & status)
		{
			slog(ERROR,"sql_wrap_string overflow before allocating terminator\n");
		}
	}

	run(m_resize(destination,required_elements));

	if(SUCCESS == status)
	{
		char *destination_data_rewritable = m_data(char,destination);

		if(destination_data_rewritable == NULL)
		{
			slog(ERROR,"sql_wrap_string destination pointer is NULL after m_resize\n");
			status = FAILURE;
		} else {
			size_t write_index = 0;

			/* Opening SQL quote */
			destination_data_rewritable[write_index++] = '\'';

			for(size_t i = 0; i < source_length; ++i)
			{
				const char current_char = source[i];

				if(current_char == '\'')
				{
					/* Double apostrophes for SQL-compatible escaping */
					destination_data_rewritable[write_index++] = '\'';
					destination_data_rewritable[write_index++] = '\'';
				} else {
					destination_data_rewritable[write_index++] = current_char;
				}
			}

			/* Closing SQL quote. write_index now equals the visible wrapped length */
			destination_data_rewritable[write_index++] = '\'';
			run(m_finalize_string(destination,write_index,WRITE_TERMINATOR_ALWAYS));
		}
	}

	provide(status);
}
