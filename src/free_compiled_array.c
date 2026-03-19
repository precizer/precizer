#include "precizer.h"

/**
 * @brief Free a NULL-terminated pcre2_code pointer array and reset the caller-owned pointer
 *
 * @param[in,out] array_ptr Pointer to the array pointer to free.
 *
 * @note Passing `NULL` or a pointer to `NULL` is allowed
 */
void free_compiled_array(pcre2_code ***array_ptr)
{
	if(array_ptr == NULL || *array_ptr == NULL)
	{
		return;
	}

	pcre2_code **compiled_patterns = *array_ptr;

	for(size_t i = 0; compiled_patterns[i] != NULL; i++)
	{
		pcre2_code_free(compiled_patterns[i]);
		compiled_patterns[i] = NULL;
	}

	free(compiled_patterns);
	*array_ptr = NULL;
}
