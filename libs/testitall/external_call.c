#include "testitall.h"

// Allocate memory for the structure char
GSTRUCT(mem_char,STDOUT)
GSTRUCT(mem_char,STDERR)
GSTRUCT(mem_char,EXTEND)

extern char **environ;  // Environment variable used by posix_spawnp

/**
 * Executes an external command and captures its stdout and stderr output.
 * @param command The shell command to execute.
 * @return The exit status SUCCESS or FAILURE on error.
 * @suppress_stderr Suppress the output from the STDERR buffer
 * @suppress_stdout Suppress the output from the STDOUT buffer
 */
Return external_call(
	const char *command,
	const int expected_return_code,
	bool suppress_stderr,
	bool suppress_stdout
){
	// Create pipes for capturing stdout and stderr
	int stdout_pipe[2], stderr_pipe[2];
	if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
		serp("pipe error");
		return(FAILURE);
	}

	// Purge data from previous usage
	del_char(&STDERR);
	// Purge data from previous usage
	del_char(&STDOUT);

	// Initialize spawn file actions and attributes
	posix_spawn_file_actions_t actions;
	posix_spawn_file_actions_init(&actions);

	// Redirect child process stdout and stderr to the pipes
	posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], STDERR_FILENO);

	// Close unused ends of pipes in the child process
	posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
	posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);
	posix_spawn_file_actions_addclose(&actions, stdout_pipe[1]);
	posix_spawn_file_actions_addclose(&actions, stderr_pipe[1]);

	pid_t pid;

	// Prepare command arguments
	char * const arguments[] = {
		(char *)(uintptr_t)"sh",
		(char *)(uintptr_t)"-c",
		(char *)(uintptr_t)command,
		NULL
	};

	// Execute command while inheriting current environment variables
	if (posix_spawnp(&pid, (char *)(uintptr_t)"sh", &actions, NULL, arguments, environ) != 0)
	{
		serp("posix_spawnp error"); // Handle command execution error
		posix_spawn_file_actions_destroy(&actions);
		return(FAILURE);
	}

	// Clean up spawn resources
	posix_spawn_file_actions_destroy(&actions);

	// Close pipe write ends
	close(stdout_pipe[1]);
	close(stderr_pipe[1]);

	// Переменные для чтения пайпа
	char *tmp_stdout_buffer = NULL; // Указатель на буфер
	size_t total_read = 0;          // Общее количество прочитанных данных
	ssize_t count = 0;              // Количество прочитанных байт за итерацию

	// Чтение данных из пайпа
	char temp_buffer[PAGE_BYTES];
	while((count = read(stdout_pipe[0], temp_buffer, PAGE_BYTES)) > 0)
	{
		if (count == -1) {
			serp("read error"); // Handle command execution error
			free(tmp_stdout_buffer);
			return(FAILURE);
		}

		// Перевыделяем память с учетом новых данных
		char *new_buffer = realloc(tmp_stdout_buffer, total_read + (size_t)count + 1); // +1 для нуля в конце
		if (!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes", total_read + (size_t)count + 1);
			free(tmp_stdout_buffer);
			return(FAILURE);
		}
		tmp_stdout_buffer = new_buffer;

		// Копируем прочитанные данные в буфер
		memcpy(tmp_stdout_buffer + total_read, temp_buffer, (size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		realloc_char(STDOUT,total_read + 1);
		memcpy(STDOUT->mem, tmp_stdout_buffer, STDOUT->length * sizeof(char));
	}

	free(tmp_stdout_buffer); // Освобождаем память

	// Null-terminate output buffer
	if(STDOUT->length > 0)
	{
		STDOUT->mem[STDOUT->length - 1] = '\0';
	}

	// Переменные для чтения
	char *tmp_stderr_buffer = NULL; // Указатель на буфер
	total_read = 0;                 // Общее количество прочитанных данных

	// Чтение данных из пайпа

	// Очистка временного буфера
	memset(temp_buffer,0,PAGE_BYTES);

	while ((count = read(stderr_pipe[0], temp_buffer, PAGE_BYTES)) > 0)
	{
		if (count == -1)
		{
			serp("read error"); // Handle command execution error
			free(tmp_stderr_buffer);
			return(FAILURE);
		}

		// Перевыделяем память с учетом новых данных
		char *new_buffer = realloc(tmp_stderr_buffer, total_read + (size_t)count + 1); // +1 для нуля в конце
		if (!new_buffer)
		{
			report("Memory allocation failed, requested size: %zu bytes", total_read + (size_t)count + 1);
			free(tmp_stderr_buffer);
			return(FAILURE);
		}
		tmp_stderr_buffer = new_buffer;

		// Копируем прочитанные данные в буфер
		memcpy(tmp_stderr_buffer + total_read, temp_buffer, (size_t)count);
		total_read += (size_t)count;
	}

	if(total_read > 0)
	{
		realloc_char(STDERR,total_read + 1);
		memcpy(STDERR->mem, tmp_stderr_buffer, STDERR->length * sizeof(char));
	}

	free(tmp_stderr_buffer);

	// Null-terminate output buffer
	if(STDERR->length > 0)
	{
		STDERR->mem[STDERR->length - 1] = '\0';
	}

	// Wait for the child process to complete and capture its exit status
	int return_code;
	if(waitpid(pid, &return_code, 0) == -1)
	{
		serp("waitpid error");
		return(FAILURE);
	}
	int exit_code = WEXITSTATUS(return_code);

	close(stdout_pipe[0]); // Закрываем конец для чтения
	close(stderr_pipe[0]);

	if(STDERR->length > 0)
	{
		// Suppress the output from the STDERR buffer or not
		if(suppress_stderr == true)
		{
			// Suppress the output from the STDERR buffer
			del_char(&STDERR);

		} else {
#if 1
			// Format stderr output
			char *str;
			int rt = asprintf(&str, \
				YELLOW "Warning! STDERR buffer is not empty!\n"
				"External command call:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
				"Stderr output:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
				command,STDERR->mem);

			if (rt > -1) {
				// Copy str to STDERR->mem
				if(SUCCESS == realloc_char(STDERR, (size_t)rt + 1))
				{
					memcpy(STDERR->mem, str, STDERR->length * sizeof(STDERR->mem[0]));
				}

			} else {
				report("Memory allocation failed, requested size: %zu bytes", (size_t)rt + 1);
			}

			free(str);
#endif
			return(FAILURE);
		}
	}

	if(STDOUT->length > 0)
	{
		// Suppress the output from the STDOUT buffer or not
		if(suppress_stdout == true)
		{
			// Suppress the output from the STDOUT buffer
			del_char(&STDOUT);

		}
	}

	// Check the exit status of the child process
	if(expected_return_code != exit_code)
	{
		// Format stderr output
		char *str;
		int rt = asprintf(&str, YELLOW "ERROR: Unexpected exit code!" RESET "\n"
			YELLOW "External command call:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
			YELLOW "Exited with code " RESET "%d " YELLOW "but expected " RESET "%d\n"
			YELLOW "Process terminated signal " RESET "%d\n"
			YELLOW "Stderr output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
			YELLOW "Stdout output:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
			command,
			exit_code,
			expected_return_code,
			WTERMSIG(return_code),
			STDERR->mem,
			STDOUT->mem);

		if (rt > -1) {
			// Copy str to STDERR->mem
			if(SUCCESS == realloc_char(STDERR,(size_t)rt + 1))
			{
				memcpy(STDERR->mem, str, STDERR->length * sizeof(STDERR->mem[0]));
			}

		} else {
			report("Memory allocation failed, requested size: %zu bytes", (size_t)rt + 1);
		}

		free(str);

		return(FAILURE);
	}

	return(SUCCESS);
}
