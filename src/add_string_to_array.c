#include "precizer.h"

/**
 * @brief Adds a new string to a dynamically allocated NULL-terminated array of strings.
 *
 * @details This function dynamically appends a string to an array of strings.
 *          The array must be NULL-terminated to allow safe iteration.
 *          It reallocates memory to accommodate the new entry, ensuring the array
 *          remains NULL-terminated after adding the new string.
 *
 * @param array_ptr  Pointer to the dynamic array of strings (NULL-terminated).
 *                   The array will be reallocated to fit the new string.
 * @param new_string The string to append to the array.
 *
 * @return SUCCESS if the string was successfully added.
 *         FAILURE if memory allocation fails at any point.
 *
 * @note The provided array must be initialized properly as either NULL or a valid
 *       NULL-terminated array of strings before calling this function.
 *
 * @warning Memory allocated by this function is freed by calling the free_config()
 *          function upon program termination.
 */
Return add_string_to_array(
	char       ***array_ptr,
	const char *new_string)
{
	/// The status that will be passed to provide() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(array_ptr == NULL)
	{
		report("Invalid parameter: array_ptr is NULL");
		provide(FAILURE);
	}

	if(new_string == NULL)
	{
		report("Invalid parameter: new_string is NULL");
		provide(FAILURE);
	}

	// Calculate the size of the current string array
	size_t size = 0;
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

		free_string_array(array);
		*array_ptr = NULL;
		provide(FAILURE);
	} else {
		array = tmp;
	}

	// Allocate memory for the new string and copy the new string into it
	array[size] = (char *)malloc((strlen(new_string) + 1) * sizeof(char));

	if(array[size] == NULL)
	{
		report("Memory allocation failed, requested size: %zu bytes",(strlen(new_string) + 1) * sizeof(char));

		// Reallocation failed, free the original array
		free_string_array(array);
		*array_ptr = NULL;
		provide(FAILURE);
	}

	strcpy(array[size],new_string);

	// Set the last element to NULL
	array[size + 1] = NULL;

	// Update the array pointer in the calling function
	*array_ptr = array;

	provide(status);
}
