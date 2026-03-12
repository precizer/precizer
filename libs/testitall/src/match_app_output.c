#include "testitall.h"

/**
 * @brief Runs the application and matches its stdout against a template file.
 *
 * @param arguments Command-line arguments passed to the application (without the binary name).
 * @param filename Path to the template file.
 * @param template Placeholder in the template to replace before matching.
 * @param replacement Value that substitutes the placeholder.
 * @param expected_return_code Expected application exit code.
 *
 * @return SUCCESS when template preparation, runit(), and match_pattern() succeed;
 *         FAILURE otherwise.
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

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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
	ASSERT(SUCCESS == runit(arguments,result,NULL,expected_return_code,ALLOW_BOTH));

	// Compare application output against modified template
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	#if 0
	write_to_temp_file(result_text ? result_text : "");
	#endif

	// Final cleanup
	call(del(result));
	call(del(pattern));

	deliver(status);
}
