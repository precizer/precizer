#include "testitall.h"

/**
 * @brief Runs the tested application and matches its stdout against a template file.
 *
 * @param arguments Command-line arguments passed to the application (without the binary name).
 * @param filename Path to the file that stores the output template/pattern.
 * @param template Placeholder in the template that should be replaced before matching.
 * @param replacement Value that substitutes the placeholder (e.g., dynamic DB names).
 * @param expected_return_code Exit code that the application is expected to return.
 *
 * @return SUCCESS when the template is loaded, the placeholder is replaced, the
 *         application exits with the expected code, and its stdout matches the pattern.
 *         FAILURE if any of these stages fails.
 *
 * @note Combines several steps:
 *       1. Reads the template from disk.
 *       2. Performs placeholder substitution.
 *       3. Invokes the application via runit() and captures stdout.
 *       4. Matches the captured output against the prepared pattern with match_pattern().
 */
Return match_app_output(
	const char *arguments,
	const char *filename,
	const char *template,
	const char *replacement,
	const int  expected_return_code)
{
	if(!arguments || !filename || !template || !replacement)
	{
		echo(STDERR,"NULL pointer passed to match_file_template\n");
		return FAILURE;
	}

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Will store template content from file
	create(char,pattern);

	// Create memory for the output
	create(char,result);

	// Read template pattern from file
	status = get_file_content(filename,pattern);

	// Replace template placeholder with actual value
	ASSERT(SUCCESS == replace_placeholder(pattern,template,replacement));

	// Execute the application and capture output
	ASSERT(SUCCESS == runit(arguments,result,expected_return_code,false,false));

	// Compare application output against modified template
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	#if 0
	write_to_temp_file(result_text ? result_text : "");
	#endif

	// Final cleanup
	call(del(result));
	call(del(pattern));

	return(status);
}
