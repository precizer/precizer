/**
 * @file execute_command.c
 * @brief Implementation of command execution functionality with output capture
 * @details This file contains implementation of execute_command function that allows
 *          executing shell commands with controlled output capture
 */

#include "testitall.h"

/**
 * @brief Executes a shell command and optionally copies captured stdout into result.
 *
 * @param command The shell command to execute (must not be NULL)
 * @param result Managed memory buffer to receive stdout (can be NULL to ignore stdout)
 * @param expected_return_code Expected exit code from the command execution
 * @param buffer_policy Bitmask controlling stdout/stderr handling.
 *                      Use STDOUT_SUPPRESS/STDERR_SUPPRESS to drop buffers.
 *
 * @return SUCCESS when the command runs, exits with the expected code, and parameters are valid;
 *         FAILURE otherwise.
 *
 * @details The function clears the shared STDOUT buffer, delegates execution to external_call(),
 *          copies any captured stdout into the provided buffer, and then frees the shared buffer.
 */
Return execute_command(
	const char   *command,
	memory       *result,
	const int    expected_return_code,
	unsigned int buffer_policy)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Validate input parameters */
	if(!command)
	{
		return(FAILURE); // Invalid arguments
	}

	/* Clean the STDOUT buffer to prepare for new command output */
	call(del(STDOUT));

	/* Execute the command with specified parameters */
	run(external_call(command,expected_return_code,buffer_policy));

	/* Copy captured output to result buffer */
	if(NULL != result)
	{
		if(STDOUT->length > 0U)
		{
			call(copy(result,STDOUT));
		}
	}

	/* Free temporary STDOUT buffer after copying */
	call(del(STDOUT));

	return(status);
}
