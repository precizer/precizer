#include "testitall.h"

/**
 * @brief Executes a command and sets its stdout as an environment variable.
 *
 * @param variable Environment variable name (must not be NULL).
 * @param command Command to execute (must not be NULL).
 * @param expected_return_code Expected exit code from the command execution.
 *
 * @return SUCCESS if execute_command() succeeds and stdout is non-empty; FAILURE otherwise.
 *
 * @note Trailing EOL in stdout is trimmed before setting the variable.
 */
Return execute_and_set_variable(
	const char *variable,
	const char *command,
	const int  expected_return_code)
{
	if(!variable || !command)
	{
		serp("NULL pointer passed to execute_and_set_variable");
		return FAILURE;
	}

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	// Create memory for storing command output
	m_create(char,result,MEMORY_STRING);

	// Execute command and capture output
	call(execute_command(command,result,NULL,expected_return_code,ALLOW_BOTH));

	if(SUCCESS == status)
	{
		// Only set environment variable if we got some output
		if(result->string_length > 0U)
		{
			run(trim_trailing_eol(result));

			run(set_environment_variable(variable,m_text(result)));

		} else {
			// Empty output is considered a failure
			echo(STDERR,"Command produced no output: %s\n",command);
			status = FAILURE;
		}
	}

	// Cleanup allocated memory
	call(m_del(result));

	deliver(status);
}
