#include "sute.h"

/**
 * @brief Verify `--maxdepth` behavior for initial traversal and later update
 *
 * @return Return status code
 */
Return test0018_1(void)
{
	INITTEST;

	/* File system traversal with a maximum depth of 3 */
	m_create(char,result,MEMORY_STRING);

	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0018_001_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--maxdepth=3 --database=database3.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	/* File system traversal with unlimited depth */
	filename = "templates/0018_001_2.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database4.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

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

	/* File system traversal with a maximum depth of 3 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	/* File system traversal with unlimited depth */
	const char *filename = "templates/0018_002.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=database3.db "
	        "$TMPDIR/tests/fixtures/levels";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

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
 * @brief Reject an empty value passed to `--maxdepth`
 *
 * `--maxdepth=` does not contain a number and must fail argument parsing instead
 * of silently selecting depth zero
 *
 * @return Return status code
 */
static Return test0018_5(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--maxdepth= tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,result,FAILURE,STDERR_ALLOW));

	const char *filename = "templates/0018_005.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 *
 * --maxdepth argument testing
 *
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
