/**
 * @file function_capture.c
 * @brief Functionality for redirecting stdout and stderr streams to buffers
 */

#include "testitall.h"

/**
 * @brief Executes a function and captures both stdout and stderr output
 *
 * @param func Function pointer to the function to be executed
 * @param stdout_buffer char MEMORY_STRING descriptor to store captured stdout output
 * @param stderr_buffer char MEMORY_STRING descriptor to store captured stderr output
 * @return Return Status of execution (SUCCESS/FAILURE)
 *
 * @details This function:
 *   1. Creates temporary files to capture stdout and stderr
 *   2. Redirects stdout and stderr to these files
 *   3. Executes the provided function
 *   4. Captures both outputs into the provided memory structures
 *   5. Restores original stdout and stderr
 *   6. Cleans up resources
 */
Return function_capture(
	void ( *func )(void),
	memory *stdout_buffer,
	memory *stderr_buffer)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int stdout_fd = -1;
	int stderr_fd = -1;
	FILE *stdout_tmp = NULL;
	FILE *stderr_tmp = NULL;

	if(SUCCESS == status)
	{
		status = m_resize(stdout_buffer,0);
	}

	if(SUCCESS == status)
	{
		status = m_resize(stderr_buffer,0);
	}

	/* Flush pending output before redirecting streams */
	if(SUCCESS == status && (fflush(stdout) != 0 || fflush(stderr) != 0))
	{
		slog(ERROR,"Failed to flush streams before redirect\n");
		status = FAILURE;
	}

	/* Save original file descriptors */
	if(SUCCESS == status)
	{
		stdout_fd = dup(STDOUT_FILENO);
		stderr_fd = dup(STDERR_FILENO);

		if(stdout_fd == -1 || stderr_fd == -1)
		{
			slog(ERROR,"Failed to save original file descriptors\n");
			status = FAILURE;
		}
	}

	/* Create temporary files for redirection */
	if(SUCCESS == status)
	{
		stdout_tmp = tmpfile();
		stderr_tmp = tmpfile();

		if(stdout_tmp == NULL || stderr_tmp == NULL)
		{
			slog(ERROR,"Failed to create temporary files for redirection\n");
			status = FAILURE;
		}
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

	/* Redirect streams */
	if(SUCCESS == status)
	{
		int stdout_tmp_fd = fileno(stdout_tmp);

		if(stdout_tmp_fd == -1 || dup2(stdout_tmp_fd,STDOUT_FILENO) == -1)
		{
			slog(ERROR,"Failed to redirect stdout stream\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int stderr_tmp_fd = fileno(stderr_tmp);

		if(stderr_tmp_fd == -1 || dup2(stderr_tmp_fd,STDERR_FILENO) == -1)
		{
			slog(ERROR,"Failed to redirect stderr stream\n");
			status = FAILURE;
		}
	}

	/* Execute target function */
	if(SUCCESS == status)
	{
		func();

		if(fflush(stdout) != 0 || fflush(stderr) != 0)
		{
			slog(ERROR,"Failed to flush redirected streams\n");
			status = FAILURE;
		}
	}

	/* Restore original streams */
	if(stdout_fd != -1)
	{
		if(dup2(stdout_fd,STDOUT_FILENO) == -1)
		{
			slog(ERROR,"Failed to restore original stdout stream\n");
			status = FAILURE;
		}
	}

	if(stderr_fd != -1)
	{
		if(dup2(stderr_fd,STDERR_FILENO) == -1)
		{
			slog(ERROR,"Failed to restore original stderr stream\n");
			status = FAILURE;
		}
	}

	/* Read data from temporary files */
	if(SUCCESS == status)
	{
		size_t stdout_size = 0;
		size_t stderr_size = 0;
		long stdout_position = 0;
		long stderr_position = 0;

		/* Get buffer sizes */
		if(fseek(stdout_tmp,0,SEEK_END) != 0 ||
		        fseek(stderr_tmp,0,SEEK_END) != 0)
		{
			slog(ERROR,"Failed to seek captured stream files\n");
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			stdout_position = ftell(stdout_tmp);
			stderr_position = ftell(stderr_tmp);

			if(stdout_position < 0 || stderr_position < 0)
			{
				slog(ERROR,"Failed to measure captured stream files\n");
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{
			stdout_size = (size_t)stdout_position;
			stderr_size = (size_t)stderr_position;
		}

		/* Allocate memory for buffers */
		char *stdout_data_rewritable = NULL;
		char *stderr_data_rewritable = NULL;

		if(SUCCESS == status)
		{
			if(stdout_size > 0)
			{
				status = m_resize(stdout_buffer,stdout_size + 1);

				if(SUCCESS == status)
				{
					stdout_data_rewritable = m_data(char,stdout_buffer);

					if(stdout_data_rewritable == NULL)
					{
						status = FAILURE;
					}
				}
			}
		}

		if(SUCCESS == status)
		{
			if(stderr_size > 0)
			{
				status = m_resize(stderr_buffer,stderr_size + 1);

				if(SUCCESS == status)
				{
					stderr_data_rewritable = m_data(char,stderr_buffer);

					if(stderr_data_rewritable == NULL)
					{
						status = FAILURE;
					}
				}
			}
		}

		/* Read data */
		if(SUCCESS == status)
		{
			if(fseek(stdout_tmp,0,SEEK_SET) != 0 ||
			        fseek(stderr_tmp,0,SEEK_SET) != 0)
			{
				slog(ERROR,"Failed to rewind captured stream files\n");
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{

			size_t read_stdout = 0;

			if(stdout_size > 0)
			{
				read_stdout = fread(stdout_data_rewritable,1,stdout_size,stdout_tmp);
			}

			size_t read_stderr = 0;

			if(stderr_size > 0)
			{
				read_stderr = fread(stderr_data_rewritable,1,stderr_size,stderr_tmp);
			}

			if(read_stdout != stdout_size || read_stderr != stderr_size)
			{
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				if(read_stdout > 0 && stdout_data_rewritable != NULL)
				{
					status = m_finalize_string(stdout_buffer,
						stdout_size,
						WRITE_TERMINATOR_ALWAYS);
				}

				if(SUCCESS == status && read_stderr > 0 && stderr_data_rewritable != NULL)
				{
					status = m_finalize_string(stderr_buffer,
						stderr_size,
						WRITE_TERMINATOR_ALWAYS);
				}
			}
		}
	}

	if(stdout_tmp != NULL)
	{
		if(fclose(stdout_tmp) != 0)
		{
			status = FAILURE;
		}
	}

	if(stderr_tmp != NULL)
	{
		if(fclose(stderr_tmp) != 0)
		{
			status = FAILURE;
		}
	}

	if(stdout_fd != -1)
	{
		if(close(stdout_fd) == -1)
		{
			status = FAILURE;
		}
	}

	if(stderr_fd != -1)
	{
		if(close(stderr_fd) == -1)
		{
			status = FAILURE;
		}
	}

	deliver(status);
}
