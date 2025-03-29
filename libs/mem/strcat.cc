/**
 * @file strcat.cc
 * @brief Template to concatenating two strings stored in memory structures.
 *        Requires both memory blocks to contain null-terminated
 *        strings (`'\0'` at the end).
 */
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(destination->length > 0)
	{
		size_t i = destination->length;

		while(i--)
		{
			if(destination->mem[i] == '\0')
			{
				destination->length = i;
			} else {
				break;
			}
		}
	}

	status = APPEND_TYPE(destination,source);

	return(status);
}
