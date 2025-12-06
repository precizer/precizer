#include "sute.h"

Return prefix_path_with_apostrophe_test(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name starts
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db tests/examples/\\'apostrophe";

	const char *filename = "templates/0024_001.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};rm database1.db;",COMPLETED,false,false));

	RETURN_STATUS;
}

Return another_prefix_path_with_apostrophe_test(void)
{
	INITTEST;

	create(char,pattern);

	// Create memory for the result
	create(char,result);

	/*
	 * Checking how paths are handled when the directory name ends
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db tests/examples/apostrophe\\'";

	const char *filename = "templates/0024_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,false,false));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};rm database1.db;",COMPLETED,false,false));

	RETURN_STATUS;
}

/**
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/\'apostrophe/\'apostrophe/
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/examples/apostrophe\'/\'apostrophe/apostrophe\'/
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return adding_and_comparing_with_apostrophe(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--progress --database=database1.db tests/examples/\\'apostrophe/\\'apostrophe/";

	// Create memory for the result
	create(char,result);
	create(char,chunk);

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--progress --database=database2.db tests/examples/apostrophe\\'/\\'apostrophe/apostrophe\\'/";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,false,false));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	// Create memory for the result
	create(char,pattern);

	const char *filename = "templates/0024_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db database2.db",COMPLETED,false,false));

	del(pattern);
	del(chunk);
	del(result);

	RETURN_STATUS;
}

// Main test runner
Return test0024(void)
{
	INITTEST;

	TEST(prefix_path_with_apostrophe_test,"Prefix path with apostrophe…");
	TEST(another_prefix_path_with_apostrophe_test,"Another prefix and apostrophe combination in the name…");
	TEST(adding_and_comparing_with_apostrophe,"Adding and comparing with apostrophe…");

	RETURN_STATUS;
}
