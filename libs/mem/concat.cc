/**
 * @file concat.cc
 * @brief Concatenates a string from a memory structure with a constant C-style string.
 * @detailed This template handles the concatenation of a null-terminated string stored
 * in a memory structure with a constant string literal (`const char *`).
 *
 * Both input strings must be null-terminated (`'\0'`) to ensure safe operation.
 * The destination buffer must have enough space to hold the combined result,
 * including the null terminator.
 */
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(source != NULL)
	{
		/* Length of the line, and \0 (EOL char) */
		size_t length = strlen(source) + 1;

		create_mem(mem_char,appending);

		status = REALLOC_TYPE(appending,length);

		if(SUCCESS == status)
		{
			snprintf(appending->mem,appending->length * sizeof(TYPE),"%s",source);

			status = STRCAT_TYPE(destination,appending);
		}

		DEL_TYPE(&appending);
	}

	return(status);
}
