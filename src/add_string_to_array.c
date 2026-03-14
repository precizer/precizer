#include "precizer.h"

/**
 * @brief Append a string to a dynamically allocated NULL-terminated string array
 *
 * @details The array must remain NULL-terminated to allow safe iteration
 *          The function grows the array as needed and stores the resulting pointer
 *          back through `array_ptr`
 *
 * @param[in,out] array_ptr Pointer to the caller-owned array pointer
 * @param[in] new_string String to append
 *
 * @return SUCCESS if the string was appended successfully
 * @return FAILURE if validation or allocation fails
 *
 * @note `*array_ptr` may be `NULL` on input
 *
 * @warning On allocation failure the function frees the current array and sets
 *          `*array_ptr` to `NULL`
 */
Return add_string_to_array(
	char       ***array_ptr,
	const char *new_string)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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

	// Measure the current array length
	size_t size = 0;
	char **array = *array_ptr;

	if(array != NULL)
	{
		while(array[size] != NULL)
		{
			size++;
		}
	}

	// Use a temporary variable so realloc() cannot overwrite the only live pointer
	char **tmp = (char **)realloc(array,(size + 2) * sizeof(char *));

	if(tmp == NULL)
	{
		// Reallocation failed, free the current array and clear the caller-owned pointer
		report("Memory allocation failed, requested size: %zu bytes",(size + 2) * sizeof(char *));

		free_string_array(&array);
		*array_ptr = NULL;
		provide(FAILURE);
	} else {
		array = tmp;
	}

	// Allocate memory for the appended string
	array[size] = (char *)malloc((strlen(new_string) + 1) * sizeof(char));

	if(array[size] == NULL)
	{
		report("Memory allocation failed, requested size: %zu bytes",(strlen(new_string) + 1) * sizeof(char));

		// String allocation failed, free the current array and clear the caller-owned pointer
		free_string_array(&array);
		*array_ptr = NULL;
		provide(FAILURE);
	}

	strcpy(array[size],new_string);

	// Keep the array NULL-terminated
	array[size + 1] = NULL;

	// Publish the possibly reallocated array back to the caller
	*array_ptr = array;

	provide(status);
}
