#include "precizer.h"

void add_string_to_array(
	char ***array_ptr,
	char *new_string
){
	// Calculate the size of the current string array
	size_t size = 0;
	char **array = *array_ptr;
	if (array != NULL) {
		while (array[size] != NULL) {
			size++;
		}
	}

	// Increase the size of the array by 1 and copy existing strings into it
	array = (char **)realloc(array, (size + 2) * sizeof(char *));
	if (array == NULL) {
		fprintf(stderr, "realloc error\n");
		exit(EXIT_FAILURE);
	}

	// Allocate memory for the new string and copy the new string into it
	array[size] = (char *)malloc((strlen(new_string) + 1) * sizeof(char));
	if (array[size] == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(EXIT_FAILURE);
	}
	strcpy(array[size], new_string);

	// Set the last element to NULL
	array[size + 1] = NULL;

	// Update the array pointer in the calling function
	*array_ptr = array;
}
