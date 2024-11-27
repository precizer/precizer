/**
 * @file append.d
 * @brief Template for concatenates the contents of two structures
 */
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/**
	 * @brief Validate input pointers
	 * @details Checks if both destination and source pointers are non-null
	 * @warning Sets status to FAILURE if either pointer is NULL
	 */
	if (!source || !source->mem)
	{
		status = FAILURE; // Add null pointer check
	}

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
