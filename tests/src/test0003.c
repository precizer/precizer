#include "sute.h"

/**
 *
 * Check the name of the database created by default.
 * Does it really comply with to the "hostname.db" template
 *
 */
Return test0003(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--progress tests/examples/diffs/diff1",NULL,COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--progress --database=database2.db tests/examples/diffs/diff2",NULL,COMPLETED,false,false));

	if(SUCCESS == status)
	{
		// Get the output of the application
		const char *arguments = "--compare ${DBNAME} database2.db";

		const char *filename = "templates/0003.txt";  // File name
		const char *template = "%DB_NAME%";

		const char *replacement = getenv("DBNAME");  // Database name

		if(replacement == NULL)
		{
			echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
			status = FAILURE;
		}

		ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));
	}

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\" \"${TMPDIR}/database2.db\"",COMPLETED,false,false));

	RETURN_STATUS;
}
