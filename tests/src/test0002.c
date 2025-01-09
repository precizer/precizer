#include "sute.h"

/**
 *
 * Create default name database
 *
 */
Return test0002(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Get the output of an external program
	const char *command = "export TESTING=true;" \
	        "cd ${TMPDIR};" \
	        "export ASAN_OPTIONS;" \
	        "export ASAN_SYMBOLIZER_PATH;" \
	        "./precizer tests/examples/diffs";

	const char *filename = "templates/0002.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",0,false,false));

	RETURN_STATUS;
}
