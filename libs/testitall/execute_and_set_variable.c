#include "testitall.h"

/**
 * @brief Executes a command and sets its output as an environment variable
 *
 * @param variable Name of the environment variable to set
 * @param command Command to execute
 * @param expected_return_code Expected return code from the command execution
 *
 * @return SUCCESS if command executed successfully and variable was set
 *         FAILURE if command failed or output was empty
 *
 * @note The function allocates memory for storing command output temporarily
 * @note Environment variable will only be set if command produces non-empty output
 */
Return execute_and_set_variable(
	const char *variable,
	const char *command,
	const int  expected_return_code
){
	if(!variable || !command)
	{
		serp("NULL pointer passed to execute_and_set_variable");
		return FAILURE;
	}

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Create memory for storing command output
	MSTRUCT(mem_char,result);

	// Execute command and capture output
	status = execute_command(command,result,expected_return_code,false,false);

	if(SUCCESS == status)
	{
		// Only set environment variable if we got some output
		if(result->length > 0)
		{
			status = set_environment_variable(variable,result->mem);

		} else {
			// Empty output is considered a failure
			echo(STDERR,"Command produced no output: %s",command);
			status = FAILURE;
		}
	}

	// Cleanup allocated memory
	del_char(&result);

	return(status);
}
