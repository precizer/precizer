#include "sute.h"

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

	const char *arguments = "--progress tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--progress --database=database2.db "
	        "tests/examples/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Get the output of the application
	arguments = "--compare $DBNAME database2.db";

	const char *filename = "templates/0003_001.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/${DBNAME}\" \"${TMPDIR}/database2.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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

	const char *arguments = "";

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);

	const char *filename = "templates/0003_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,FAILURE,STDERR_ALLOW));

	ASSERT(SUCCESS == get_file_content(filename,stderr_pattern));

	// Match stderr against the pattern
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern,filename));

	filename = "templates/0003_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));

	// Match stdout against the pattern
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	// Clean to use it iteratively
	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	RETURN_STATUS;

}

Return test0003(void)
{
	INITTEST;

	TEST(test0003_1,"Comply default DB name to \"hostname.db\" template…");
	TEST(test0003_2,"Running the application with no arguments at all…");

	RETURN_STATUS;
}
