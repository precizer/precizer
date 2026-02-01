/**
 * @file execute_command.c
 * @brief Implementation of command execution functionality with output capture
 * @details This file contains implementation of execute_command function that allows
 *          executing shell commands with controlled output capture
 */

#include "testitall.h"

/**
 * @brief Executes a shell command and captures stdout/stderr into provided buffers.
 *
 * @param command Shell command to execute (must not be NULL).
 * @param stdout_result Buffer to receive stdout (NULL to ignore).
 * @param stderr_result Buffer to receive stderr (NULL to ignore).
 * @param expected_return_code Expected exit code from the command execution.
 * @param buffer_policy Bitmask controlling stdout/stderr handling (see capture_policy).
 *
 * @return SUCCESS when external_call() succeeds; FAILURE otherwise.
 */
Return execute_command(
	const char   *command,
	memory       *stdout_result,
	memory       *stderr_result,
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
	run(external_call(command,stdout_result,stderr_result,expected_return_code,buffer_policy));

	/* Free temporary STDOUT buffer after copying */
	call(del(STDOUT));

	return(status);
}
