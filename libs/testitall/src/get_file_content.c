#include "testitall.h"

/**
 * @brief Reads entire contents of a file into memory
 *
 * @param filename Path to file to read
 * @param pattern Managed memory buffer that will be resized to hold the file content
 *
 * @return Return status:
 *         SUCCESS if file was read successfully
 *         FAILURE if file couldn't be opened or read, memory allocation failed, or file is empty

 * @note Caller is responsible for owning and later freeing the provided memory descriptor with m_del()
 * @note Function resizes the buffer to the file size plus a terminating null byte
 */
Return get_file_content(
	const char *filename,
	memory     *pattern)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	long file_size_long = 0;
	size_t file_size = 0;
	size_t read_size = 0;

	// Validate input parameters
	if((filename == NULL) || (pattern == NULL))
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
		status = m_resize(pattern,file_size + 1);
	}

	// Read file content
	if(SUCCESS == status)
	{
		char *pattern_data_rewritable = m_data(char,pattern);

		if(pattern_data_rewritable == NULL)
		{
			status = FAILURE;
		} else {
			read_size = fread(pattern_data_rewritable,1,file_size,file);

			if(read_size != file_size)
			{
				echo(STDERR,"Failed to read file (expected %zu, got %zu bytes): %s\n",
					file_size,read_size,filename);
				status = FAILURE;
			} else {
				status = m_finalize_string(pattern,file_size,WRITE_TERMINATOR_ALWAYS);
			}
		}
	}

	// Cleanup
	if(file)
	{
		fclose(file);
	}

	if(status != SUCCESS)
	{
		m_del(pattern);
	}

	deliver(status);
}
