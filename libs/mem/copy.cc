/**
 * @file copy.d
 * @brief Memory copy template for generic types
 * @details Reallocates destination structure memory and copies data from source structure
 *          Handles memory reallocation and copying for different data types using preprocessor macros
 */
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/**
	 * @brief Validate input pointers
	 * @details Checks if both destination and source pointers are non-null
	 * @warning If either pointer is NULL don't interrupt but nothing to copy
	 */
	if (!source || !source->mem)
	{
		return(SUCCESS); // Nothing to copy
	}

	if(SUCCESS == status)
	{

		if(source->length > 0)
		{
			status = REALLOC_TYPE(destination,source->length);
		} else {
			// Zero length and cleared memory
			status = DEL_TYPE(&destination);
		}
	}

	if(SUCCESS == status)
	{

		memcpy(destination->mem,source->mem,source->length * sizeof(TYPE));
	}

	return(status);
}
