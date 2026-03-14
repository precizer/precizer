#include "precizer.h"

/**
 * @brief Free a NULL-terminated string array and reset the caller-owned pointer
 *
 * @param[in,out] array_ptr Pointer to the array pointer to free
 *
 * @note Passing `NULL` or a pointer to `NULL` is allowed
 */
void free_string_array(char ***array_ptr)
{
	if(array_ptr == NULL || *array_ptr == NULL)
	{
		return;
	}

	char **array = *array_ptr;

	for(size_t i = 0; array[i] != NULL; i++)
	{
		free(array[i]);
		array[i] = NULL;
	}

	free(array);
	*array_ptr = NULL;
}
