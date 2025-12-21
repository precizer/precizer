#include "sute.h"

/**
 *
 * Testing database creation attempt in missing directory
 *
 */
Return test0020_1(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0020_001.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--update --database=nonexistent_directory/database1.db tests/examples/diffs/diff1",result,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Testing attempt to open DB with --update when database is missing
 *
 */
Return test0020_2(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0020_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--update --database=nonexistent_database1.db tests/examples/diffs/diff1",result,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Testing DB creation in write protected directory
 *
 */
Return test0020_3(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *command = "cd ${TMPDIR} && "
	                "mkdir write_protected_directory && "
	                "chmod a-rwx write_protected_directory";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));


	const char *filename = "templates/0020_003.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--database=write_protected_directory/database1.db tests/examples/diffs/diff1",result,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && chmod a+rwx write_protected_directory && rm -df write_protected_directory",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Testing attempt to open DB with write protected database file
 *
 */
Return test0020_4(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--database=write_protected_database1.db tests/examples/diffs/diff1",NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == runit("--database=database2.db tests/examples/diffs/diff2",NULL,COMPLETED,ALLOW_BOTH));

	const char *command = "cd ${TMPDIR} && "
	                "chmod a-rwx write_protected_database1.db";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0020_004.txt";

	ASSERT(SUCCESS == runit("--compare write_protected_database1.db database2.db",result,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && rm database2.db",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Testing attempt to update DB with --update when database file is write protected
 *
 */
Return test0020_5(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *filename = "templates/0020_005.txt";

	ASSERT(SUCCESS == runit("--update --database=write_protected_database1.db tests/examples/diffs/diff1",result,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && rm -f write_protected_database1.db",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Attempt to change the primary path in the database
 *
 */
Return test0020_6(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0020_006.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--database=database1.db tests/examples/diffs/diff1",result,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == runit("--update --database=database1.db tests/examples/diffs/diff2",result,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Replace the primary path in the database
 *
 */
Return test0020_7(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0020_007.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--update --force --database=database1.db tests/examples/diffs/diff2",result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && rm -f database1.db",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

Return test0020(void)
{
	INITTEST;

	TEST(test0020_1,"DB creation in missing directory…")
	TEST(test0020_2,"Attempt to open DB with --update when database is missing…")
	TEST(test0020_3,"DB creation in write protected directory…")
	TEST(test0020_4,"Attempt to open DB with write protected database file…")
	TEST(test0020_5,"Attempt to update DB with --update when database file is write protected…")
	TEST(test0020_6,"Attempt to change the primary path in the database…")
	TEST(test0020_7,"Replace the primary path in the database…")

	RETURN_STATUS;
}
