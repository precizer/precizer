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

	ASSERT(SUCCESS == runit("--update --database=nonexistent_directory/database1.db tests/examples/diffs/diff1",result,FAILURE,false,false));

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
 * Testing database creation attempt with --update and missing database
 *
 */
Return test0020_2(void)
{
	INITTEST;

	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0020_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--update --database=nonexistent_database1.db tests/examples/diffs/diff1",result,FAILURE,false,false));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

Return test0020(void)
{

	INITTEST;

	TEST(test0020_1,"DB creation in missing directory…")
	TEST(test0020_2,"Creation attempt with --update and missing database……")

	RETURN_STATUS;
}
