#include "precizer.h"

/**
 * @brief Free a NULL-terminated array of strings.
 *
 * @param array Array of strings to free.
 */
void free_string_array(char **array)
{
	if(array == NULL)
	{
		return;
	}

	for(size_t i = 0; array[i] != NULL; i++)
	{
		free(array[i]);
		array[i] = NULL;
	}

	free(array);
	array = NULL;
}
