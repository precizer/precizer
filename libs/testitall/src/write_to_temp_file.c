#include "testitall.h"

/**
 * @brief Writes content of a buffer to a temporary file
 * @param buffer Pointer to the null-terminated string to be written
 * @return Return enum value indicating execution status
 * @note Creates a temporary file using mkstemp with template /tmp/testitall_XXXXXX
 */
Return write_to_temp_file(const char *buffer)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	int fd = -1;
	FILE *temp_file = NULL;

	if(SUCCESS == status)
	{
		if(buffer == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		char template[] = "/tmp/testitall_XXXXXX";

		fd = mkstemp(template);

		if(fd == -1)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		temp_file = fdopen(fd,"w");

		if(temp_file == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int write_result = fprintf(temp_file,"%s",buffer);

		if(write_result < 0)
		{
			status = FAILURE;
		}
	}

	// Cleanup
	if(temp_file != NULL)
	{
		fclose(temp_file); // This also closes fd
	} else if(fd != -1){
		close(fd);
	}

	deliver(status);
}
