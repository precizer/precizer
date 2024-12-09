#include "testitall.h"

/**
 * @brief Executes command and matches its output against a template from file
 *
 * @param command Command to execute
 * @param filename File containing the template pattern
 * @param template Template placeholder to replace
 * @param replacement String to replace template placeholder with
 * @param expected_return_code Expected return code from command execution
 *
 * @return SUCCESS if command output matches modified template
 *         FAILURE if any step fails (file read, placeholder replacement, command execution, pattern match)
 *
 * @note Function combines multiple operations:
 *       1. Reads template from file
 *       2. Replaces placeholder in template
 *       3. Executes command
 *       4. Matches command output against modified template
 */
Return match_file_template(
	const char *command,
	const char *filename,
	const char *template,
	const char *replacement,
	const int expected_return_code
){
	if (!command || !filename || !template || !replacement) {
		echo(STDERR,"NULL pointer passed to match_file_template");
		return FAILURE;
	}

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Will store template content from file
	char *pattern = NULL;

	// Create memory for command output
	MSTRUCT(mem_char, result);

	// Read template pattern from file
	status = get_file_content(filename, &pattern);

	// Replace template placeholder with actual value
	if(SUCCESS == status)
	{
		status = replace_placeholder(&pattern, template, replacement);
	}

	// Execute command and capture output
	if(SUCCESS == status)
	{
		status = execute_command(command, result, expected_return_code);
	}

	// Compare command output against modified template
	if(SUCCESS == status)
	{
		status = match_pattern(result->mem, pattern);
	}

	// Final cleanup
	del_char(&result);
	free(pattern);
	pattern = NULL;

	return(status);
}
