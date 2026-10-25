#include "sute.h"

/**
 * @brief Verify `--maxdepth` behavior for initial traversal and later update
 *
 * Both depth-limited and unlimited traversals are repeated through configuration
 * from an empty database. The depth-limited database remains for the later update
 *
 * @return Return status code
 */
Return test0018_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		/* File system traversal with a maximum depth of 3 */
		{"--maxdepth=3 --database=database3.db "
		 "$TMPDIR/tests/fixtures/levels",COMPLETED,"templates/0018_001_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Repeat the limited traversal from the same empty state */
	ASSERT(SUCCESS == delete_path("database3.db"));
	ASSERT(SUCCESS == copy_path("tests/templates/0018_001_1.conf","0018_001_1.conf"));
	ASSERT(SUCCESS == replace_placeholder_in_file("$TMPDIR/0018_001_1.conf","%ROOT_PATH%","$TMPDIR/tests/fixtures/levels"));
	const struct argument_test_case file_cases[] = {
		{"--config=0018_001_1.conf",COMPLETED,"templates/0018_001_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	call(delete_path_if_present("0018_001_1.conf"));

	const struct argument_test_case unlimited_cases[] = {
		/* File system traversal with unlimited depth */
		{"--database=database4.db "
		 "$TMPDIR/tests/fixtures/levels",COMPLETED,"templates/0018_001_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(unlimited_cases,NULL,NULL));

	/* Repeat the unlimited traversal, starting without its database */
	ASSERT(SUCCESS == delete_path("database4.db"));
	ASSERT(SUCCESS == copy_from_origin("tests/templates/0018_001_2.conf","0018_001_2.conf",REQUIRE_SOURCE_EXISTS));
	ASSERT(SUCCESS == replace_placeholder_in_file("$TMPDIR/0018_001_2.conf","%ROOT_PATH%","$TMPDIR/tests/fixtures/levels"));
	const struct argument_test_case unlimited_file_cases[] = {
		{"--config=0018_001_2.conf",COMPLETED,"templates/0018_001_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(unlimited_file_cases,NULL,NULL));

	call(delete_path_if_present("0018_001_2.conf"));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database4.db"));

	RETURN_STATUS;
}

/**
 * @brief Verify update output after rerunning the maxdepth fixture without the depth limit
 *
 * @return Return status code
 */
Return test0018_2(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* File system traversal with unlimited depth */
	const struct argument_test_case cases[] = {
		{"--update --check-level=QUICK --database=database3.db "
		 "$TMPDIR/tests/fixtures/levels",COMPLETED,"templates/0018_002.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database3.db"));

	RETURN_STATUS;
}

/**
 * @brief Verify template diff generation through compare_memory_strings
 *
 * This test loads the two saved outputs produced by the depth-limited and
 * unlimited traversal scenarios into string-mode memory descriptors. It then
 * compares those descriptors through compare_memory_strings() and stores the
 * resulting unified diff directly in another string-mode descriptor
 *
 * The test verifies that compare_memory_strings():
 * - accepts file content loaded into string-mode memory descriptors
 * - writes the produced unified diff directly into a destination descriptor
 * - produces the exact diff text expected by template `0018_003`
 *
 * @return Return status code
 */
Return test0018_3(void)
{
	INITTEST;

	m_create(char,text1,MEMORY_STRING);
	m_create(char,text2,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,diff_buffer,MEMORY_STRING);

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_001_2.txt",text2));

	ASSERT(SUCCESS == compare_memory_strings(diff_buffer,text1,text2));

	const char *filename = "templates/0018_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	m_del(text1);
	m_del(text2);
	m_del(pattern);
	m_del(diff_buffer);

	RETURN_STATUS;
}

/**
 * @brief Verify updated traversal diff generation through compare_memory_strings
 *
 * This test loads the original depth-limited traversal output together with the
 * later `--update` output into string-mode memory descriptors. It compares them
 * through compare_memory_strings() and checks the produced unified diff against
 * the expected template `0018_004`
 *
 * The test verifies that compare_memory_strings():
 * - accepts saved traversal outputs as string-mode memory descriptors
 * - stores the unified diff directly in the destination descriptor
 * - produces the exact diff text expected for the update scenario
 *
 * @return Return status code
 */
Return test0018_4(void)
{
	INITTEST;

	m_create(char,text1,MEMORY_STRING);
	m_create(char,text2,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,diff_buffer,MEMORY_STRING);

	/* 0018 001 */
	ASSERT(SUCCESS == get_file_content("templates/0018_001_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0018_002.txt",text2));

	ASSERT(SUCCESS == compare_memory_strings(diff_buffer,text1,text2));

	const char *filename = "templates/0018_004.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	m_del(text1);
	m_del(text2);
	m_del(pattern);
	m_del(diff_buffer);

	RETURN_STATUS;
}

/**
 * @brief Reject an empty maxdepth value through CLI and configuration
 *
 * `--maxdepth=` does not contain a number and must fail argument parsing instead
 * of silently selecting depth zero. An empty configuration value must also
 * fail and report its line number
 *
 * @return Return status code
 */
static Return test0018_5(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--maxdepth= tests/fixtures/diffs/diff1",FAILURE,"templates/0018_005_1.txt","templates/0018_005_2.txt"},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0018_005.conf",FAILURE,"templates/0018_005_3.txt","templates/0018_005_4.txt"},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * @brief Check traversal depth and explicit database paths
 * @return Return status for traversal, output comparisons, and database filenames
 */
Return test0018(void)
{
	INITTEST;

	TEST(test0018_1,"Traversal with limited depth");
	TEST(test0018_2,"Update DB w/o depth limits");
	TEST(test0018_3,"Comparing templates w/ and w/o depth limits")
	TEST(test0018_4,"Comparing templates after update w/o depth limits")
	TEST(test0018_5,"Empty --maxdepth value should fail argument parsing")

	RETURN_STATUS;
}
