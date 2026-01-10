#include "sute.h"

/**
 * "inaccessible" message of show_relative_path() function
 */
static Return test0029_1_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "chmod a-rx tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Change file permitions
	        "chmod a-rx tests/examples/diffs/diff1/2/AAA/BBB/CZC;"; // Change directory permitions

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db && "
		"rm -rf tests/examples/ && "
		"mv tests/examples_backup/ tests/examples/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 *
 *
 */
Return test0029(void)
{
	INITTEST;

	TEST(test0029_1_test,"\"inaccessible\" message of show_relative_path() function…");

	RETURN_STATUS;
}
