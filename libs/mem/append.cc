/**
 * @file append.d
 * @brief Template for concatenates the contents of two structures
 */
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	size_t destination_prev_size = destination->length;
	size_t new_length = destination->length + source->length;

	if(SUCCESS == status)
	{
		status = REALLOC_TYPE(destination,new_length);
	}

	if(SUCCESS == status)
	{

		memcpy(destination->mem + destination_prev_size,
			source->mem,
			source->length * sizeof(TYPE)
		);
	}

	return(status);
}
