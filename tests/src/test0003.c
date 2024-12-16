#include "sute.h"

/**
 *
 * Check the name of the database created by default.
 * Does it really comply with to the "hostname.db" template
 *
 */
Return test0003(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	ASSERT(SUCCESS == external_call("export TESTING=true;cd ${TMPDIR};precizer --progress tests/examples/diffs/diff1",0));

	ASSERT(SUCCESS == external_call("export TESTING=true;cd ${TMPDIR};precizer --progress --database=database2.db tests/examples/diffs/diff2",0,false,true));

	if(SUCCESS == status)
	{

		// Get the output of an external program
		const char *command = "export TESTING=true;cd ${TMPDIR};precizer --compare ${DBNAME} database2.db";

		const char *filename = "templates/0003.txt";  // File name
		const char *template = "%DB_NAME%";

		const char *replacement = getenv("DBNAME");  // Database name

		if(replacement == NULL)
		{
			echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
			return(FAILURE);
		}

		ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));
	}

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\" \"${TMPDIR}/database2.db\"",0));

	RETURN_STATUS;
}
