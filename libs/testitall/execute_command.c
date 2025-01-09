/**
 * @file execute_command.c
 * @brief Implementation of command execution functionality with output capture
 * @details This file contains implementation of execute_command function that allows
 *          executing shell commands with controlled output capture
 */

#include "testitall.h"

/**
 * @brief Executes a shell command and optionally captures its output
 *
 * @param command The shell command to execute (must not be NULL)
 * @param result Buffer to store command output (can be NULL if output capture not needed)
 * @param expected_return_code Expected return code from the command execution
 * @param suppress_stderr If true, stderr output will be suppressed
 * @param suppress_stdout If true, stdout output will be suppressed
 *
 * @return Return enumeration value:
 *         - SUCCESS if command executed successfully and returned expected code
 *         - FAILURE if command failed or parameters were invalid
 *
 * @details The function executes the given shell command while managing output streams.
 *          It preserves the original STDOUT content by creating a temporary copy,
 *          executes the command, captures the output if requested, and restores
 *          the original STDOUT state.
 *
 * @note This function uses the external_call() function for actual command execution
 * @warning The command parameter must not be NULL
 */
Return execute_command(
	const char *command,
	mem_char *result,
	const int expected_return_code,
	bool suppress_stderr,
	bool suppress_stdout
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Validate input parameters */
	if (!command)
	{
		status = FAILURE; // Invalid arguments
	}

	// Clean the STDOUT buffer to prepare for new command output
	status = del_char(&STDOUT);

	/* Execute the command with specified parameters */
	if(SUCCESS == status)
	{
		status = external_call(command, expected_return_code, suppress_stderr, suppress_stdout);
	}

	/* Process command output if needed */
	if(SUCCESS == status)
	{
		// Only process output if there is content and result buffer provided
		if(STDOUT->length > 0 && result != NULL)
		{
			// Remove trailing newline if present
			if(STDOUT->length > 1)
			{
				// Reduce buffer size by 1 to remove EOL character
				status = realloc_char(STDOUT,STDOUT->length - 1);

				if(SUCCESS == status)
				{
					// Ensure proper string termination
					STDOUT->mem[STDOUT->length - 1] = '\0';
				}
			}

			/* Copy captured output to result buffer */
			if(SUCCESS == status)
			{
				status = copy_char(result,STDOUT);
			}

			/* Clean up temporary STDOUT storage */
			if(SUCCESS == status)
			{
				// Free temporary STDOUT buffer after copying
				status = del_char(&STDOUT);
			}
		}
	}

	return(status);
}
