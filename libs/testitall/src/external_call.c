#include "testitall.h"
#include <sysexits.h>

// Global buffers for captured output streams
memory _STDOUT = {sizeof(char),0,0,NULL};
memory *STDOUT = &_STDOUT;
memory _STDERR = {sizeof(char),0,0,NULL};
memory *STDERR = &_STDERR;
memory _EXTEND = {sizeof(char),0,0,NULL};
memory *EXTEND = &_EXTEND;

extern char **environ; // Environment variables used by posix_spawnp

/**
 * Executes an external command, capturing stdout/stderr into shared buffers.
 * @param command              Shell command to execute.
 * @param expected_return_code Exit code the command must produce for SUCCESS.
 * @param buffer_policy        Bitmask controlling stdout/stderr handling.
 *                             Use STDOUT_SUPPRESS/STDERR_SUPPRESS to drop buffers,
 *                             STDOUT_ENABLE/STDERR_ENABLE to document enabled streams,
 *                             and their combinations like ALLOW_BOTH.
 * @return SUCCESS if execution, capture, and exit code checks succeed; FAILURE otherwise.
 *
 * Side effects:
 *  - Clears global STDOUT/STDERR buffers before running.
 *  - Formats diagnostics into STDERR on unexpected exit codes or unsuppressed stderr.
 */
Return external_call(
	const char   *command,
	const int    expected_return_code,
	unsigned int buffer_policy)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;
	const bool suppress_stdout = (buffer_policy & STDOUT_SUPPRESS) != 0U;
	const bool suppress_stderr = (buffer_policy & STDERR_SUPPRESS) != 0U;

	// Create pipes to capture stdout and stderr
	int stdout_pipe[2],stderr_pipe[2];

	if(pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1)
	{
		serp("Error creating pipe");
		return(FAILURE);
	}

	// Clear data from previous usage
	del(STDERR);
	// Clear data from previous usage
	del(STDOUT);

	// Initialize spawn file actions and attributes
	posix_spawn_file_actions_t actions;
	posix_spawn_file_actions_init(&actions);

	// Redirect child process stdout and stderr to the pipes
	posix_spawn_file_actions_adddup2(&actions,stdout_pipe[1],STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&actions,stderr_pipe[1],STDERR_FILENO);

	// Close unused ends of the pipes in the child process
	posix_spawn_file_actions_addclose(&actions,stdout_pipe[0]);
	posix_spawn_file_actions_addclose(&actions,stderr_pipe[0]);
	posix_spawn_file_actions_addclose(&actions,stdout_pipe[1]);
	posix_spawn_file_actions_addclose(&actions,stderr_pipe[1]);

	pid_t pid;

	// Prepare command arguments
	char *const arguments[] = {
		(char *)(uintptr_t)"sh",
		(char *)(uintptr_t)"-c",
		(char *)(uintptr_t)command,
		NULL
	};

	// Execute the command while inheriting current environment variables
	if(posix_spawnp(&pid,(char *)(uintptr_t)"sh",&actions,NULL,arguments,environ) != 0)
	{
		serp("Error executing posix_spawnp"); // Handle command execution error
		posix_spawn_file_actions_destroy(&actions);
		return(FAILURE);
	}

	// Clean up spawn resources
	posix_spawn_file_actions_destroy(&actions);

	// Close the write ends of the pipes
	close(stdout_pipe[1]);
	close(stderr_pipe[1]);

	// Variables for reading from the pipe
	char *tmp_stdout_buffer = NULL; // Pointer to the buffer
	size_t total_read = 0;      // Total bytes read so far
	ssize_t count = 0;          // Bytes read during each iteration

	// Read data from the pipe (chunk size matches libmem's MEMORY_BLOCK_BYTES)
	char temp_buffer[MEMORY_BLOCK_BYTES];

	while((count = read(stdout_pipe[0],temp_buffer,MEMORY_BLOCK_BYTES)) > 0)
	{
		if(count == -1)
		{
			serp("Error reading from pipe"); // Handle read error
			free(tmp_stdout_buffer);
			return(FAILURE);
		}

		// Reallocate memory to accommodate new data
		char *new_buffer = realloc(tmp_stdout_buffer,total_read + (size_t)count + 1); // +1 for null terminator

		if(!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes",total_read + (size_t)count + 1);
			free(tmp_stdout_buffer);
			return(FAILURE);
		}
		tmp_stdout_buffer = new_buffer;

		// Copy the read data into the buffer
		memcpy(tmp_stdout_buffer + total_read,temp_buffer,(size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		if(resize(STDOUT,total_read + 1) != SUCCESS)
		{
			free(tmp_stdout_buffer);
			return(FAILURE);
		}

		char *stdout_mem = data(char,STDOUT);

		if(stdout_mem == NULL)
		{
			free(tmp_stdout_buffer);
			return(FAILURE);
		}

		memcpy(stdout_mem,tmp_stdout_buffer,(size_t)total_read);
		stdout_mem[STDOUT->length - 1] = '\0';
	}

	free(tmp_stdout_buffer); // Free allocated memory

	// Variables for reading stderr
	char *tmp_stderr_buffer = NULL; // Pointer to the buffer
	total_read = 0;             // Total bytes read so far

	/* Read data from the pipe */

	// Clear the temporary buffer
	memset(temp_buffer,0,MEMORY_BLOCK_BYTES);

	while((count = read(stderr_pipe[0],temp_buffer,MEMORY_BLOCK_BYTES)) > 0)
	{
		if(count == -1)
		{
			serp("Error reading from pipe"); // Handle read error
			free(tmp_stderr_buffer);
			return(FAILURE);
		}

		// Reallocate memory to accommodate new data
		char *new_buffer = realloc(tmp_stderr_buffer,total_read + (size_t)count + 1); // +1 for null terminator

		if(!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes",total_read + (size_t)count + 1);
			free(tmp_stderr_buffer);
			return(FAILURE);
		}
		tmp_stderr_buffer = new_buffer;

		// Copy the read data into the buffer
		memcpy(tmp_stderr_buffer + total_read,temp_buffer,(size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		if(resize(STDERR,total_read + 1) != SUCCESS)
		{
			free(tmp_stderr_buffer);
			return(FAILURE);
		}

		char *stderr_mem = data(char,STDERR);

		if(stderr_mem == NULL)
		{
			free(tmp_stderr_buffer);
			return(FAILURE);
		}

		memcpy(stderr_mem,tmp_stderr_buffer,(size_t)total_read);
		stderr_mem[STDERR->length - 1] = '\0';
	}

	free(tmp_stderr_buffer);

	// Wait for the child process to finish and capture its exit status
	int return_code;
	bool allow_stderr = (expected_return_code == EX_USAGE);

	if(waitpid(pid,&return_code,0) == -1)
	{
		serp("Error waiting for child process");
		return(FAILURE);
	}
	int exit_code = WEXITSTATUS(return_code);

	close(stdout_pipe[0]); // Close the read end of the pipe
	close(stderr_pipe[0]);

	if(STDERR->length > 0)
	{
		if(allow_stderr == true)
		{
			if(STDOUT->length == 0 && STDOUT->data == NULL)
			{
				run(copy(STDOUT,STDERR));

			} else {
				run(concat_strings(STDOUT,STDERR));
			}

			// Clear STDERR to avoid warning reports
			del(STDERR);

		} else {
			// Suppress the output from the STDERR buffer if needed
			if(suppress_stderr == true)
			{
				// Suppress the output from the STDERR buffer
				del(STDERR);

			} else {
				// Format stderr output
				char *str;
				const char *stderr_view = getcstring(STDERR);
				int rt = asprintf(&str, \
					YELLOW "Warning! STDERR buffer is not empty!\n"
					"External command call:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
					"Stderr output:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
					command,stderr_view);

				if(rt > -1)
				{
					// Copy str into STDERR buffer
					run(resize(STDERR,(size_t)rt + 1));

					run(copy_literal(STDERR,str));

				} else {
					report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1);
				}

				free(str);

				return(FAILURE);
			}
		}
	}

	if(STDOUT->length > 0)
	{
		// Suppress the output from the STDOUT buffer if needed
		if(suppress_stdout == true)
		{
			// Suppress the output from the STDOUT buffer
			del(STDOUT);

		}
	}

	// Check the exit status of the child process
	if(expected_return_code != exit_code)
	{
		// Format stderr output
		char *str;
		const char *stderr_view = getcstring(STDERR);
		const char *stdout_view = getcstring(STDOUT);
		int rt = asprintf(&str,YELLOW "ERROR: Unexpected exit code!" RESET "\n"
			YELLOW "External command call:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
			YELLOW "Exited with code " RESET "%d " YELLOW "but expected " RESET "%d\n"
			YELLOW "Process terminated signal" RESET " %d\n"
			YELLOW "Stderr output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
			YELLOW "Stdout output:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
			command,
			exit_code,
			expected_return_code,
			WTERMSIG(return_code),
			stderr_view,
			stdout_view);

		if(rt > -1)
		{
			// Copy str into STDERR buffer
			run(resize(STDERR,(size_t)rt + 1));

			run(copy_literal(STDERR,str));

		} else {
			report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1);
		}

		free(str);

		return(FAILURE);
	}

	return(SUCCESS);
}
