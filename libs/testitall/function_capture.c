/**
 * @file function_capture.c
 * @brief Functionality for redirecting stdout and stderr streams to buffers
 */

#include "testitall.h"

/**
 * @brief Executes a function and captures both stdout and stderr output
 *
 * @param func Function pointer to the function to be executed
 * @param stdout_buffer mem_char structure to store captured stdout output
 * @param stderr_buffer mem_char structure to store captured stderr output
 * @return Return Status of execution (SUCCESS/FAILURE)
 *
 * @details This function:
 *   1. Creates temporary files to capture stdout and stderr
 *   2. Redirects stdout and stderr to these files
 *   3. Executes the provided function
 *   4. Captures both outputs into the provided mem_char structures
 *   5. Restores original stdout and stderr
 *   6. Cleans up resources
 */
Return function_capture(
	void (*func)(void),
	mem_char *stdout_buffer,
	mem_char *stderr_buffer
){
	Return status = SUCCESS;

	/* Save original file descriptors */
	int stdout_fd = dup(STDOUT_FILENO);
	int stderr_fd = dup(STDERR_FILENO);

	if(stdout_fd == -1 || stderr_fd == -1)
	{
		slog(ERROR,"Failed to save original file descriptors\n");
		return FAILURE;
	}

	/* Create temporary files for redirection */
	FILE *stdout_tmp = tmpfile();
	FILE *stderr_tmp = tmpfile();

	if(stdout_tmp == NULL || stderr_tmp == NULL)
	{
		slog(ERROR,"Failed to create temporary files for redirection\n");
		status = FAILURE;
	}

	/* Disable buffering for temporary files */
	if(SUCCESS == status)
	{
		if(setvbuf(stdout_tmp,NULL,_IONBF,0) != 0 ||
		        setvbuf(stderr_tmp,NULL,_IONBF,0) != 0)
		{
			slog(ERROR,"Failed to disable buffering\n");
			status = FAILURE;
		}
	}

	/* Disable buffering for stdout and stderr */
	if(SUCCESS == status)
	{
		if(setvbuf(stdout,NULL,_IONBF,0) != 0 ||
		        setvbuf(stderr,NULL,_IONBF,0) != 0)
		{
			slog(ERROR,"Failed to disable buffering\n");
			status = FAILURE;
		}
	}

	/* Redirect streams */
	if(SUCCESS == status)
	{
		if(dup2(fileno(stdout_tmp),STDOUT_FILENO) == -1 ||
		        dup2(fileno(stderr_tmp),STDERR_FILENO) == -1)
		{
			slog(ERROR,"Failed to redirect streams\n");
			status = FAILURE;
		}
	}

	/* Execute target function */
	if(SUCCESS == status)
	{
		func();
		fflush(stdout);
		fflush(stderr);
	}

	/* Restore original streams */
	if(SUCCESS == status)
	{
		if(dup2(stdout_fd,STDOUT_FILENO) == -1 ||
		        dup2(stderr_fd,STDERR_FILENO) == -1)
		{
			slog(ERROR,"Failed to restore original streams\n");
			status = FAILURE;
		}
	}

	/* Read data from temporary files */
	if(SUCCESS == status)
	{
		size_t stdout_size,stderr_size;

		/* Get buffer sizes */
		fseek(stdout_tmp,0,SEEK_END);
		fseek(stderr_tmp,0,SEEK_END);
		stdout_size = (size_t)ftell(stdout_tmp);
		stderr_size = (size_t)ftell(stderr_tmp);

		/* Allocate memory for buffers */
		if(SUCCESS == status)
		{
			if(stdout_size > 0)
			{
				status = realloc_char(stdout_buffer,stdout_size + 1);
			}
		}

		if(SUCCESS == status)
		{
			if(stderr_size > 0)
			{
				status = realloc_char(stderr_buffer,stderr_size + 1);
			}
		}

		/* Read data */
		if(SUCCESS == status)
		{
			fseek(stdout_tmp,0,SEEK_SET);
			fseek(stderr_tmp,0,SEEK_SET);

			size_t read_stdout = 0;

			if(stdout_size > 0)
			{
				read_stdout = fread(stdout_buffer->mem,1,stdout_size,stdout_tmp);
			}

			size_t read_stderr = 0;

			if(stderr_size > 0)
			{
				read_stderr = fread(stderr_buffer->mem,1,stderr_size,stderr_tmp);
			}

			if(read_stdout != stdout_size || read_stderr != stderr_size)
			{
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				if(read_stdout > 0)
				{
					stdout_buffer->mem[stdout_size] = '\0';
				}

				if(read_stderr > 0)
				{
					stderr_buffer->mem[stderr_size] = '\0';
				}
			}
		}
	}

	fclose(stdout_tmp);
	fclose(stderr_tmp);

	close(stdout_fd);
	close(stderr_fd);

	return(status);
}
