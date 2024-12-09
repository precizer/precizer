#include "testitall.h"

/**
 * @brief Reads entire contents of a file into memory
 *
 * @param filename Path to file to read
 * @param pattern Pointer to char pointer where content will be stored
 *
 * @return SUCCESS if file was read successfully
 *         FAILURE if file couldn't be opened or read, or memory allocation failed
 *
 * @note Caller is responsible for freeing the allocated memory in *pattern
 * @note Function will allocate exact amount of memory needed for file content plus null terminator
 */
Return get_file_content(
	const char *filename,
	char **pattern
){
	if (!filename || !pattern) {
		echo(STDERR,"NULL pointer passed to get_file_content");
		return(FAILURE);
	}

	*pattern = NULL;  // Initialize to NULL in case of failure

	// Open file for reading
	FILE *file = fopen(filename, "r");
	if (!file) {
		echo(STDERR,"ERROR: Failed to open pattern file\n");
		return(FAILURE);
	}

	// Get file size by seeking to end and back
	if (fseek(file, 0, SEEK_END) != 0) {
		echo(STDERR,"Failed to seek file: %s", filename);
		fclose(file);
		return(FAILURE);
	}

	long file_size_long = ftell(file);
	if (file_size_long < 0) {
		echo(STDERR,"Failed to get file size: %s", filename);
		fclose(file);
		return(FAILURE);
	}

	size_t file_size = (size_t)file_size_long;

	// Check for empty file
	if (file_size == 0) {
		echo(STDERR,"Empty file: %s", filename);
		fclose(file);
		return FAILURE;
	}

	// Check for multiplication overflow
	if (file_size > SIZE_MAX - 1) {
		echo(STDERR,"File too large: %s", filename);
		fclose(file);
		return FAILURE;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		echo(STDERR,"Failed to seek back to start: %s", filename);
		fclose(file);
		return FAILURE;
	}

	// Allocate memory for file content plus null terminator
	*pattern = (char *)malloc((file_size + 1) * sizeof(char));
	if (*pattern == NULL) {
		// Handle memory allocation failure
		echo(STDERR,"Memory allocation failed: %zu bytes", file_size + 1);
		fclose(file);
		return(FAILURE);
	}

	// Read entire file into allocated buffer
	size_t read_size = fread(*pattern, 1, file_size, file);
	if(read_size != file_size)
	{
		echo(STDERR,"Failed to read file (expected %zu, got %zu bytes): %s",
			file_size, read_size, filename);
		// Cleanup on read failure
		free(*pattern);
		*pattern = NULL;
		fclose(file);
		return(FAILURE);
	}

	// Add null terminator
	(*pattern)[file_size] = '\0';

	fclose(file);

	return(SUCCESS);
}
