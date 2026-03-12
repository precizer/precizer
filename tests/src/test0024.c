#include "sute.h"

Return test0024_1(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name starts
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/\\'apostrophe";

	const char *filename = "templates/0024_001.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));

	RETURN_STATUS;
}

Return test0024_2(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name ends
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db tests/fixtures/apostrophe\\'";

	const char *filename = "templates/0024_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));

	RETURN_STATUS;
}

/**
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/fixtures/\'apostrophe/\'apostrophe/
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/fixtures/apostrophe\'/\'apostrophe/apostrophe\'/
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return test0024_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/\\'apostrophe/\\'apostrophe/";

	// Create memory for the result
	create(char,result);
	create(char,chunk);

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--progress --database=database2.db "
	        "tests/fixtures/apostrophe\\'/\\'apostrophe/apostrophe\\'/";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == concat_strings(result,chunk));

	// Create memory for the result
	create(char,pattern);

	const char *filename = "templates/0024_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	del(pattern);
	del(chunk);
	del(result);

	RETURN_STATUS;
}

// Main test runner
Return test0024(void)
{
	INITTEST;

	TEST(test0024_1,"Prefix path with apostrophe…");
	TEST(test0024_2,"Another prefix and apostrophe combination in the name…");
	TEST(test0024_3,"Adding and comparing with apostrophe…");

	RETURN_STATUS;
}
