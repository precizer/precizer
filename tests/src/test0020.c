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

	const char *arguments = "--update --database=nonexistent_directory/database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));

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

	const char *arguments = "--update --database=nonexistent_database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));

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

	ASSERT(SUCCESS == create_directory("write_protected_directory"));
	ASSERT(SUCCESS == change_mode("write_protected_directory",0000));

	const char *filename = "templates/0020_003.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=write_protected_directory/database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == change_mode("write_protected_directory",0777));
	ASSERT(SUCCESS == delete_path("write_protected_directory"));

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

	const char *arguments = "--database=write_protected_database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == change_mode("write_protected_database1.db",0000));

	const char *filename = "templates/0020_004.txt";

	arguments = "--compare write_protected_database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == delete_path("database2.db"));

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

	const char *arguments = "--update --database=write_protected_database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == change_mode("write_protected_database1.db",0666));
	ASSERT(SUCCESS == delete_path("write_protected_database1.db"));

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

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --database=database1.db tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

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

	const char *arguments = "--update --force --database=database1.db "
	        "tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	ASSERT(SUCCESS == delete_path("database1.db"));

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
