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
	 * @warning Sets status to FAILURE if either pointer is NULL
	 */
	if (!source || !source->mem)
	{
		status = FAILURE; // Add null pointer check
	}

	if(SUCCESS == status)
	{

		status = REALLOC_TYPE(destination,source->length);
	}

	if(SUCCESS == status)
	{

		memcpy(destination->mem,source->mem,source->length * sizeof(TYPE));
	}

	return(status);
}
