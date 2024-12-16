#include "precizer.h"

/**
 * @brief Adds a new string to a NULL-terminated array of strings
 *
 * @details This function adds additional strings to a dynamic
 * array of strings. The last line contains NULL. This
 * way safely loop could be used until NULL is
 * encountered.
 *
 * @return Return SUCCESS if string was added successfully,
 *         FAILURE if memory allocation failed
 *
 * @note The array must be NULL-terminated
 *
 * @warning Freeing the memory from the string
 * array occurs in the free_config() function
 * before the app exits.
 *
 */
Return add_string_to_array(
	char       ***array_ptr,
	const char *new_string
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Calculate the size of the current string array
	size_t size  = 0;
	char **array = *array_ptr;

	if(array != NULL)
	{
		while(array[size] != NULL)
		{
			size++;
		}
	}

	// Increase the size of the array by 1 and copy existing strings into it
	// Use a temporary variable to realloc the array
	char **tmp = (char **)realloc(array,(size + 2) * sizeof(char *));

	if(tmp == NULL)
	{
		// Reallocation failed, free the original array
		report("Memory allocation failed, requested size: %zu bytes",(size + 2) * sizeof(char *));

		for(size_t i = 0; i < size; i++)
		{
			free(array[i]);
		}
		free(array);
		return(FAILURE);
	} else {
		array = tmp;
	}

	// Allocate memory for the new string and copy the new string into it
	array[size] = (char *)malloc((strlen(new_string) + 1) * sizeof(char));

	if(array[size] == NULL)
	{
		report("Memory allocation failed, requested size: %zu bytes",(strlen(new_string) + 1) * sizeof(char));

		// Reallocation failed, free the original array
		for(size_t i = 0; i < size; i++)
		{
			free(array[i]);
		}
		free(array);
		return(FAILURE);
	}

	strcpy(array[size],new_string);

	// Set the last element to NULL
	array[size + 1] = NULL;

	// Update the array pointer in the calling function
	*array_ptr = array;

	return(status);
}
