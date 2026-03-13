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
	const char *command = NULL;

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

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
	ASSERT(SUCCESS == delete_path("database1.db"));

	command = "cd ${TMPDIR} && "
	        "chmod -R a+rwX tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

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

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	const char *command = "cd ${TMPDIR};"
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
	ASSERT(SUCCESS == delete_path("database2.db"));

	command = "cd ${TMPDIR} && "
	        "chmod -R a+rwX tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

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
