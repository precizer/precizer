#include "testitall.h"

/**
 * @brief Remove one trailing EOL sequence from a byte string descriptor
 *
 * This helper accepts a `MEMORY_STRING` descriptor whose elements are `char`
 * values. It examines the visible string boundary cached in `string_length`
 * and removes one final `\r\n`, `\n`, or `\r` sequence through
 * `m_string_truncate()`, preserving the descriptor length and reusable
 * reserve
 *
 * @param buffer Byte string descriptor to update
 *
 * @return SUCCESS if trimming succeeds or no trailing EOL is present. FAILURE
 *         if @p buffer is not a valid byte string descriptor
 */
Return trim_trailing_eol(memory *buffer)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(buffer == NULL ||
	        buffer->single_element_size != sizeof(char))
	{
		deliver(FAILURE);
	}

	/* m_string_truncate() accepts an empty descriptor before checking this stale-cache case */
	if(buffer->length == 0U && buffer->string_length != 0U)
	{
		deliver(FAILURE);
	}

	/* Use the cached visible string boundary instead of the available descriptor span */
	const size_t len = buffer->string_length;

	/* Ask libmem to validate the current string descriptor and its terminator */
	run(m_string_truncate(buffer,len));

	/* An empty visible string has no trailing EOL to remove */
	if((TRIUMPH & status) && len == 0U)
	{
		deliver(SUCCESS);
	}

	if(TRIUMPH & status)
	{
		const char *buffer_data_view = m_text(buffer);
		size_t new_len = len;

		/* Check for Windows EOL (\r\n) */
		if(len >= 2U && buffer_data_view[len - 2U] == '\r' && buffer_data_view[len - 1U] == '\n')
		{
			new_len = len - 2U;

		} else if(buffer_data_view[len - 1U] == '\n' || buffer_data_view[len - 1U] == '\r'){
			/* Single \n or \r */
			new_len = len - 1U;
		}

		/* Change means one trailing EOL was found and should be removed */
		if(new_len != len)
		{
			/* Preserve the existing logical reserve while shortening the visible string */
			run(m_string_truncate(buffer,new_len));
		}
	}

	deliver(status);
}
