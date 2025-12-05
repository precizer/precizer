#include "testitall.h"

/**
 * @brief Remove a trailing EOL (\r, \n, or \r\n sequence) from a memory buffer.
 *
 * @param buffer Pointer to a memory descriptor containing text.
 *
 * @return SUCCESS if trimming succeeded or nothing to trim; FAILURE on invalid input.
 *
 * @details
 * - Works with buffers that already have a trailing '\0' or without it.
 * - Removes a Windows EOL pair (\r\n) at the end if present.
 * - Removes a single \n or \r at the end if present.
 * - Does nothing for empty or very short buffers.
 */
Return trim_trailing_eol(
	memory *buffer)
{
	Return status = SUCCESS;

	if(buffer == NULL)
	{
		return(FAILURE);
	}

	/* Nothing to trim if length is less than one byte */
	if(buffer->length < 1U)
	{
		return(SUCCESS);
	}

	size_t len = buffer->length;
	char *data_ptr = data(char,buffer);

	if(data_ptr == NULL)
	{
		return(FAILURE);
	}

	/* If there is a trailing '\0', operate on the actual string payload */
	if(len > 0U && data_ptr[len - 1U] == '\0')
	{
		len -= 1U;
	}

	/* After trimming a possible '\0' we might end up empty */
	if(len == 0U)
	{
		return(SUCCESS);
	}

	size_t new_len = len;

	/* Check for Windows EOL (\r\n) */
	if(len >= 2U && data_ptr[len - 2U] == '\r' && data_ptr[len - 1U] == '\n')
	{
		new_len = len - 2U;

	} else if(data_ptr[len - 1U] == '\n' || data_ptr[len - 1U] == '\r'){
		/* Single \n or \r */
		new_len = len - 1U;
	}

	/* No change means no trailing EOL */
	if(new_len == len)
	{
		return(SUCCESS);
	}

	/* Restore trailing '\0' if it originally existed */
	const bool had_null_terminator = (buffer->length > 0U && data_ptr[buffer->length - 1U] == '\0');
	size_t target_length = new_len;

	if(had_null_terminator)
	{
		/* Reserve +1 for the terminator */
		target_length = new_len + 1U;
	}

	run(resize(buffer,target_length));

	if(SUCCESS == status && had_null_terminator)
	{
		char *ptr = data(char,buffer);

		if(ptr == NULL)
		{
			status = FAILURE;
		} else {
			ptr[target_length - 1U] = '\0';
		}
	}

	provide(status);
}
