#include "sute.h"

/**
 * "inaccessible" message of file_show() function
 */
static Return test0029_1(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0666));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0777));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * --db-drop-inaccessible option. Dropping DB records for inaccessible paths
 */
static Return test0029_2(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --db-drop-inaccessible --database=database2.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0666));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0777));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

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

	TEST(test0029_1,"\"inaccessible\" message of file_show() function");
	TEST(test0029_2,"--db-drop-inaccessible option. Dropping DB records for inaccessible paths");

	RETURN_STATUS;
}
