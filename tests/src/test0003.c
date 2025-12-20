#include "sute.h"
#include <sysexits.h>

/**
 *
 * Check the name of the database created by default.
 * Does it really comply with to the "hostname.db" template
 *
 */
Return test0003_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--progress tests/examples/diffs/diff1",NULL,COMPLETED,false,false));

	ASSERT(SUCCESS == runit("--progress --database=database2.db tests/examples/diffs/diff2",NULL,COMPLETED,false,false));

	// Get the output of the application
	const char *arguments = "--compare ${DBNAME} database2.db";

	const char *filename = "templates/0003_001.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\" \"${TMPDIR}/database2.db\"",COMPLETED,false,false));

	RETURN_STATUS;
}

/**
 *
 * Running the application with no arguments at all
 *
 */
Return test0003_2(void)
{
	INITTEST;

#if 0
	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0003_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("",result,EX_USAGE,false,false));
#endif

	ASSERT(SUCCESS == runit("",NULL,EX_USAGE,STDERR_SUPPRESS,STDOUT_ENABLE));

#if 0
	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
#endif

	RETURN_STATUS;

}

Return test0003(void)
{
	INITTEST;

	TEST(test0003_1,"Comply default DB name to \"hostname.db\" template…");
	TEST(test0003_2,"Running the application with no arguments at all…");

	RETURN_STATUS;
}
