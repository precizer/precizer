#include "testitall.h"

/**
 * @brief Reads entire contents of a file into memory
 *
 * @param filename Path to file to read
 * @param pattern Pointer to char pointer where content will be stored
 *
 * @return Return status:
 *         SUCCESS if file was read successfully
 *         FAILURE if file couldn't be opened or read, or memory allocation failed

 * @note Caller is responsible for freeing the allocated memory in *pattern
 * @note Function will allocate exact amount of memory needed for file content plus null terminator
 */
Return get_file_content(
	const char *filename,
	char       **pattern
){
	Return status = SUCCESS;
	FILE *file = NULL;
	*pattern = NULL; // Initialize to NULL in case of failure
	long file_size_long = 0;
	size_t file_size = 0;
	size_t read_size = 0;

	// Validate input parameters
	if(!filename || !pattern)
	{
		echo(STDERR,"NULL pointer passed to get_file_content\n");
		status = FAILURE;
	}

	// Initialize output parameter
	if(SUCCESS == status)
	{
		// Open file for reading
		file = fopen(filename,"r");

		if(!file)
		{
			echo(STDERR,"Failed to open pattern file %s\n",filename);
			status = FAILURE;
		}
	}

	// Get file size
	if(SUCCESS == status)
	{
		if(fseek(file,0,SEEK_END) != 0)
		{
			echo(STDERR,"Failed to seek file: %s\n",filename);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		file_size_long = ftell(file);

		if(file_size_long < 0)
		{
			echo(STDERR,"Failed to get file size: %s\n",filename);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		file_size = (size_t)file_size_long;

		// Check for empty file
		if(file_size == 0)
		{
			echo(STDERR,"Empty file: %s\n",filename);
			status = FAILURE;
		}
	}

	// Check for multiplication overflow
	if(SUCCESS == status)
	{
		if(file_size > SIZE_MAX - 1)
		{
			echo(STDERR,"File too large: %s\n",filename);
			status = FAILURE;
		}
	}

	// Return to file start
	if(SUCCESS == status)
	{
		if(fseek(file,0,SEEK_SET) != 0)
		{
			echo(STDERR,"Failed to seek back to start: %s\n",filename);
			status = FAILURE;
		}
	}

	// Allocate memory
	if(SUCCESS == status)
	{
		*pattern = (char *)malloc((file_size + 1) * sizeof(char));

		if(*pattern == NULL)
		{
			report("Memory allocation failed: %zu bytes\n",file_size + 1);
			status = FAILURE;
		}
	}

	// Read file content
	if(SUCCESS == status)
	{
		read_size = fread(*pattern,1,file_size,file);

		if(read_size != file_size)
		{
			echo(STDERR,"Failed to read file (expected %zu, got %zu bytes): %s\n",
				file_size,read_size,filename);
			status = FAILURE;
		}
	}

	// Add null terminator if succeeded
	if(SUCCESS == status)
	{
		(*pattern)[file_size] = '\0';
	}

	// Cleanup
	if(file)
	{
		fclose(file);
	}

	if(FAILURE == status && *pattern)
	{
		free(*pattern);
		*pattern = NULL;
	}

	return(status);
}
