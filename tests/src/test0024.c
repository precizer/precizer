#include "sute.h"

/**
 * @brief Verify traversal output when the source path starts with an apostrophe
 *
 * CLI and configuration runs enable progress and use the same full output
 * template, including the initial file count. Each run starts without a database
 *
 * @return Return status code
 */
Return test0024_1(void)
{
	INITTEST;

	/*
	 * Checking how paths are handled when the directory name starts
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/\\'apostrophe";

	const char *filename = "templates/0024_001.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));

	/* Repeat the scan through configuration with the literal apostrophe in the path */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0024_001.conf",COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	ASSERT(SUCCESS == delete_path("database1.db"));

	RETURN_STATUS;
}

/**
 * @brief Verify traversal output when the source path ends with an apostrophe
 *
 * @return Return status code
 */
Return test0024_2(void)
{
	INITTEST;

	/*
	 * Checking how paths are handled when the directory name ends
	 * with an apostrophe
	 */
	const char *arguments = "--progress --database=database1.db tests/fixtures/apostrophe\\'";

	const char *filename = "templates/0024_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

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
 * precizer --compare --check-level=QUICK database1.db database2.db
 */
static Return test0024_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/\\'apostrophe/\\'apostrophe/";

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,chunk,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == m_copy(result,chunk));

	arguments = "--progress --database=database2.db "
	        "tests/fixtures/apostrophe\\'/\\'apostrophe/apostrophe\\'/";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	arguments = "--compare --check-level=QUICK database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	// Create memory for the result
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0024_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	m_del(pattern);
	m_del(chunk);
	m_del(result);

	RETURN_STATUS;
}

// Main test runner
Return test0024(void)
{
	INITTEST;

	TEST(test0024_1,"Prefix path with apostrophe");
	TEST(test0024_2,"Another prefix and apostrophe combination in the name");
	TEST(test0024_3,"Adding and comparing with apostrophe");

	RETURN_STATUS;
}
