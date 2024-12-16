#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0010(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Set environment variable
	ASSERT(SUCCESS == set_environment_variable("REPLACEMENT","gentl"));

	// Get the output of an external program
	const char *command = "echo 'The gentle melody floated through\n" \
	        "the afternoon air as children played in the garden,\n" \
	        "their laughter mixing with birdsong.\n" \
	        "Butterflies danced among colorful flowers while a\n"
	        "tabby cat watched lazily from its sunny spot on the\n"
	        "wooden fence, tail swaying gently in the warm summer breeze.'";

	const char *filename = "templates/0010.txt";  // File name
	const char *template = "%REPLACEMENT%";

	const char *replacement = getenv("REPLACEMENT");  // Get variable value

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable REPLACEMENT is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	RETURN_STATUS;
}
