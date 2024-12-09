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
	/* The status that will be passed to return() before exiting */
	Return status = SUCCESS;

	/* Initialize working variables */
	int stderr_fd = -1;
	int stdout_fd = -1;
	int original_stderr = -1;
	int original_stdout = -1;
	char stderr_template[] = "/tmp/testitall_stderr_XXXXXX";
	char stdout_template[] = "/tmp/testitall_stdout_XXXXXX";
	ssize_t bytes_read = 0;
	char read_buffer[BUFFER_LENGTH];

	/* Validate input parameters */
	if(stderr_buffer == NULL || stdout_buffer == NULL)
	{
		serp("Buffer pointers cannot be NULL");
		status = FAILURE;
	}

	/* Save original file descriptors */
	if(SUCCESS == status)
	{
		original_stderr = dup(STDERR_FILENO);
		original_stdout = dup(STDOUT_FILENO);
		if(original_stderr == -1 || original_stdout == -1)
		{
			serp("Failed to save original descriptors");
			status = FAILURE;
		}
	}

	/* Create temporary files */
	if(SUCCESS == status)
	{
		stderr_fd = mkstemp(stderr_template);
		stdout_fd = mkstemp(stdout_template);
		if(stderr_fd == -1 || stdout_fd == -1)
		{
			serp("Failed to create temporary files");
			status = FAILURE;
		}
	}

	/* Flush both output streams first */
	if(SUCCESS == status)
	{
		if(fflush(stdout) != 0 || fflush(stderr) != 0)
		{
			serp("Failed to flush of output streams buffers");
			status = FAILURE;
		}
	}

	/* Redirect output streams */
	if(SUCCESS == status)
	{
		if(dup2(stderr_fd, STDERR_FILENO) == -1 ||
			dup2(stdout_fd, STDOUT_FILENO) == -1)
		{
			serp("Failed to redirect streams");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Execute target function */
		func();

		/* Restore original streams */
		if(dup2(original_stderr, STDERR_FILENO) == -1 ||
			dup2(original_stdout, STDOUT_FILENO) == -1)
		{
			serp("Failed to restore streams");
			status = FAILURE;
		}
	}

	/* Process stderr */
	if(SUCCESS == status)
	{
		/* Reset file position */
		if(lseek(stderr_fd, 0, SEEK_SET) == -1)
		{
			serp("Failed to seek stderr file");
			status = FAILURE;
		}
	}

	/* Read stderr data */
	if(SUCCESS == status)
	{
		while(true)
		{
			bytes_read = read(stderr_fd, read_buffer, BUFFER_LENGTH);

			if(bytes_read == -1)
			{
				status = FAILURE;
				serp("Failed to read stderr");
				break;

			} else if(bytes_read == 0){

				/* End of file reached */
				break;
			}

			size_t old_length = stderr_buffer->length;

			/* Calculate new required size */
			size_t new_length = stderr_buffer->length + (size_t)bytes_read;

			/* Reallocate buffer if needed */
			if(SUCCESS == (status = realloc_char(stderr_buffer, new_length)))
			{
				/* Copy new data */
				memcpy(stderr_buffer->mem + old_length, read_buffer, (size_t)bytes_read);
			} else {
				serp("Failed to reallocate stderr buffer");
				break;
			}
		}

		if(stderr_buffer->length > 0 )
		{
			// Null-termination
			if(SUCCESS == (status = realloc_char(stderr_buffer, stderr_buffer->length + 1)))
			{
				stderr_buffer->mem[stderr_buffer->length - 1] = '\0';
			} else {
				serp("Failed to reallocate stderr buffer");
			}
		}
	}

	/* Process stdout */
	if(SUCCESS == status)
	{
		/* Reset file position */
		if(lseek(stdout_fd, 0, SEEK_SET) == -1)
		{
			serp("Failed to seek stdout file");
			status = FAILURE;
		}
	}

	/* Read stdout data */
	if(SUCCESS == status)
	{
		while(true)
		{
			bytes_read = read(stdout_fd, read_buffer, BUFFER_LENGTH);

			if(bytes_read == -1)
			{
				status = FAILURE;
				serp("Failed to read stdout");
				break;

			} else if(bytes_read == 0){

				/* End of file reached */
				break;
			}

			size_t old_length = stdout_buffer->length;

			/* Calculate new required size */
			size_t new_length = stdout_buffer->length + (size_t)bytes_read;

			/* Reallocate buffer if needed */
			if(SUCCESS == (status = realloc_char(stdout_buffer, new_length)))
			{
				/* Copy new data */
				memcpy(stdout_buffer->mem + old_length, read_buffer, (size_t)bytes_read);
			} else {
				serp("Failed to reallocate stdout buffer");
				break;
			}
		}

		if(stdout_buffer->length > 0 )
		{
			// Null-termination
			if(SUCCESS == (status = realloc_char(stdout_buffer, stdout_buffer->length + 1)))
			{
				stdout_buffer->mem[stdout_buffer->length - 1] = '\0';
			} else {
				serp("Failed to reallocate stdout buffer");
			}
		}
	}

	/* Cleanup */
	if(original_stderr != -1) close(original_stderr);
	if(original_stdout != -1) close(original_stdout);
	if(stderr_fd != -1)
	{
		close(stderr_fd);
		unlink(stderr_template);
	}
	if(stdout_fd != -1)
	{
		close(stdout_fd);
		unlink(stdout_template);
	}

	return(status);
}
