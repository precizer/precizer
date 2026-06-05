#include "testitall.h"

#include <spawn.h>
#include <sys/wait.h>

// Global buffers for captured output streams
memory _STDOUT = m_init_static(char,MEMORY_STRING);
memory *STDOUT = &_STDOUT;
memory _STDERR = m_init_static(char,MEMORY_STRING);
memory *STDERR = &_STDERR;
memory _EXTEND = m_init_static(char,MEMORY_STRING);
memory *EXTEND = &_EXTEND;

extern char **environ; // Environment variables used by posix_spawnp

/**
 * Executes an external command, capturing stdout/stderr into shared buffers.
 * @param command Shell command to execute.
 * @param expected_return_code Exit code the command must produce for SUCCESS.
 * @param buffer_policy Bitmask controlling stdout/stderr handling (see capture_policy).
 * @return SUCCESS when the command runs and exit code matches; FAILURE otherwise.
 *
 * On exit-code mismatch, STDERR is replaced with an error report.
 */
static Return external_call_impl(
	const char   *command,
	const int    expected_return_code,
	CAPTURE_POLICY buffer_policy)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const bool suppress_stdout = (buffer_policy & STDOUT_SUPPRESS) != 0U;
	const bool suppress_stderr = (buffer_policy & STDERR_SUPPRESS) != 0U;
	const bool allow_stderr    = (buffer_policy & STDERR_ALLOW) != 0U;

	// Create pipes to capture stdout and stderr
	int stdout_pipe[2],stderr_pipe[2];

	if(pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1)
	{
		serp("Error creating pipe");
		deliver(FAILURE);
	}

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
		deliver(FAILURE);
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
			deliver(FAILURE);
		}

		// Reallocate memory to accommodate new data
		char *new_buffer = realloc(tmp_stdout_buffer,total_read + (size_t)count + 1); // +1 for null terminator

		if(!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes",total_read + (size_t)count + 1);
			free(tmp_stdout_buffer);
			deliver(FAILURE);
		}
		tmp_stdout_buffer = new_buffer;

		// Copy the read data into the buffer
		memcpy(tmp_stdout_buffer + total_read,temp_buffer,(size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		if(m_resize(STDOUT,total_read + 1) != SUCCESS)
		{
			free(tmp_stdout_buffer);
			deliver(FAILURE);
		}

		char *stdout_data_rewritable = m_data(char,STDOUT);

		if(stdout_data_rewritable == NULL)
		{
			free(tmp_stdout_buffer);
			deliver(FAILURE);
		}

		memcpy(stdout_data_rewritable,tmp_stdout_buffer,(size_t)total_read);

		if(m_finalize_string(STDOUT,total_read,WRITE_TERMINATOR_ALWAYS) != SUCCESS)
		{
			free(tmp_stdout_buffer);
			deliver(FAILURE);
		}
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
			deliver(FAILURE);
		}

		// Reallocate memory to accommodate new data
		char *new_buffer = realloc(tmp_stderr_buffer,total_read + (size_t)count + 1); // +1 for null terminator

		if(!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes",total_read + (size_t)count + 1);
			free(tmp_stderr_buffer);
			deliver(FAILURE);
		}
		tmp_stderr_buffer = new_buffer;

		// Copy the read data into the buffer
		memcpy(tmp_stderr_buffer + total_read,temp_buffer,(size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		if(m_resize(STDERR,total_read + 1) != SUCCESS)
		{
			free(tmp_stderr_buffer);
			deliver(FAILURE);
		}

		char *stderr_data_rewritable = m_data(char,STDERR);

		if(stderr_data_rewritable == NULL)
		{
			free(tmp_stderr_buffer);
			deliver(FAILURE);
		}

		memcpy(stderr_data_rewritable,tmp_stderr_buffer,(size_t)total_read);

		if(m_finalize_string(STDERR,total_read,WRITE_TERMINATOR_ALWAYS) != SUCCESS)
		{
			free(tmp_stderr_buffer);
			deliver(FAILURE);
		}
	}

	free(tmp_stderr_buffer);

	// Wait for the child process to finish and capture its exit status
	int return_code;

	if(waitpid(pid,&return_code,0) == -1)
	{
		serp("Error waiting for child process");
		deliver(FAILURE);
	}
	int exit_code = WEXITSTATUS(return_code);

	close(stdout_pipe[0]); // Close the read end of the pipe
	close(stderr_pipe[0]);

	// Check the exit status of the child process first
	if(expected_return_code != exit_code)
	{
		// Format stderr output
		char *str;
		const char *stderr_view = m_text(STDERR);
		const char *stdout_view = m_text(STDOUT);
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
			run(m_resize(STDERR,(size_t)rt + 1));

			run(m_copy_fixed_string(STDERR,(size_t)rt + 1U,str));

		} else {
			report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1);
		}

		free(str);

		deliver(FAILURE);
	}

	if(STDERR->string_length > 0U)
	{
		if(allow_stderr == true)
		{
			// Keep STDERR contents without failing
		} else if(suppress_stderr == true){
			// Suppress the output from the STDERR buffer
			m_del(STDERR);

		} else {
			// Format stderr output
			char *str;
			const char *stderr_view = m_text(STDERR);
			int rt = asprintf(&str, \
				YELLOW "Warning! STDERR buffer is not empty!\n"
				"External command call:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
				"Stderr output:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
				command,stderr_view);

			if(rt > -1)
			{
				// Copy str into STDERR buffer
				run(m_resize(STDERR,(size_t)rt + 1));

				run(m_copy_fixed_string(STDERR,(size_t)rt + 1U,str));

			} else {
				report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1);
			}

			free(str);

			deliver(FAILURE);
		}
	}

	if(STDOUT->string_length > 0U)
	{
		// Suppress the output from the STDOUT buffer if needed
		if(suppress_stdout == true)
		{
			// Suppress the output from the STDOUT buffer
			m_del(STDOUT);

		}
	}

	deliver(SUCCESS);
}

/**
 * @brief Execute a shell command and copy captured output into caller buffers.
 *
 * Clears shared STDOUT/STDERR, runs external_call_impl(), then copies captured
 * output into stdout_result/stderr_result when provided. If STDERR_ALLOW is set
 * and the call succeeds, shared STDERR is cleared.
 */
Return external_call(
	const char   *command,
	memory       *stdout_result,
	memory       *stderr_result,
	const int    expected_return_code,
	CAPTURE_POLICY buffer_policy)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const bool allow_stderr = (buffer_policy & STDERR_ALLOW) != 0U;

	// Clear data from previous usage
	call(m_del(STDOUT));
	call(m_del(STDERR));

	run(external_call_impl(command,expected_return_code,buffer_policy));

	if(NULL != stdout_result && STDOUT->string_length > 0U)
	{
		call(m_copy(stdout_result,STDOUT));
	}

	if(NULL != stderr_result && STDERR->string_length > 0U)
	{
		call(m_copy(stderr_result,STDERR));
	}

	if(true == allow_stderr)
	{
		run(m_del(STDERR));
	}

	deliver(status);
}
