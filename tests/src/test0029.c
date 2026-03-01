#include "sute.h"

/**
 * "inaccessible" message of file_show() function
 */
static Return test0029_1(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/fixtures/diffs/diff1 tests/fixtures/diff1_backup;"
	        "cp -a tests/fixtures/diff1_backup tests/fixtures/diffs/diff1;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "chmod 000 tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"
	        "chmod 000 tests/fixtures/diffs/diff1/2/AAA/BBB/CZC;"; // Change directory permitions

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db && "
	        "chmod -R a+rwX tests/fixtures/diffs/diff1 && "
	        "rm -rf tests/fixtures/diffs/diff1 && "
	        "mv tests/fixtures/diff1_backup tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * --db-drop-inaccessible option. Dropping DB records for inaccessible paths
 */
static Return test0029_2(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/fixtures/diffs/diff1 tests/fixtures/diff1_backup;"
	        "cp -a tests/fixtures/diff1_backup tests/fixtures/diffs/diff1;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "chmod 000 tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"
	        "chmod 000 tests/fixtures/diffs/diff1/2/AAA/BBB/CZC;"; // Change directory permitions

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --db-drop-inaccessible --database=database2.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database2.db && "
	        "chmod -R a+rwX tests/fixtures/diffs/diff1 && "
	        "rm -rf tests/fixtures/diffs/diff1 && "
	        "mv tests/fixtures/diff1_backup tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Testing how the application behaves with inaccessible files
 *
 */
Return test0029(void)
{
	INITTEST;

	TEST(test0029_1,"\"inaccessible\" message of file_show() function…");
	TEST(test0029_2,"--db-drop-inaccessible option. Dropping DB records for inaccessible paths…");

	RETURN_STATUS;
}
