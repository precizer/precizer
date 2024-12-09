#include "sute.h"

/**
 * The Example 1 from README:
 * Now some tests could be running:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/examples/diffs/diff2
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return readme_example_1_test(void)
{
	Return status = SUCCESS;

	const char *command = "export TESTING=true;cd ${TMPDIR};" \
		"./precizer --progress --database=database1.db tests/examples/diffs/diff1;" \
		"./precizer --progress --database=database2.db tests/examples/diffs/diff2;" \
		"./precizer --compare database1.db database2.db";

	// Create memory for the result
	MSTRUCT(mem_char, result);

	ASSERT(SUCCESS == execute_command(command, result, 0));

	char *pattern = NULL;

	ASSERT(SUCCESS == get_file_content("templates/0011_001.txt", &pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem, pattern));

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
				"rm database1.db database2.db",0));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	RETURN_STATUS;
}

/**
 * The Example 2 from README
 * Updating the database:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 2. Reuse previous example once agan. The first try. The warning message.
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 3. Run of database update without making actual changes to disk:
 * precizer --update --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 4. Now let's make some changes:
 * # Backup
 * cp -r tests/examples/ tests/examples_backup
 * # Modify a file
 * echo -n "  " >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt
 * # Add a new file
 * touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt
 * # Remove a file
 * rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt
 * Stage 5. Run the precizer once again:
 * precizer --update --progress --database=database1.db tests/examples/diffs/diff1
 * Final stage. Recover from backup:
 * rm -rf tests/examples/
 * mv tests/examples_backup/ tests/examples/
 */
static Return readme_example_2_test(void)
{
	Return status = SUCCESS;

	// Preparation for tests
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
				"cp -r tests/examples/ tests/examples_backup;",0));

	// Main test run
	const char *command = "export TESTING=true;cd ${TMPDIR};" \
			"./precizer --progress --database=database1.db tests/examples/diffs/diff1";

	// Create memory for the result
	MSTRUCT(mem_char, result);

	char *pattern = NULL;

	ASSERT(SUCCESS == execute_command(command, result, 0));
	ASSERT(SUCCESS == get_file_content("templates/0011_002_1.txt", &pattern));
	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem, pattern));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};" \
		"./precizer --progress --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command, result, 1));
	ASSERT(SUCCESS == get_file_content("templates/0011_002_2.txt", &pattern));
	ASSERT(SUCCESS == match_pattern(result->mem, pattern));

	// Clean to use it iteratively
	free(pattern);
	pattern = NULL;
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};" \
		"./precizer --update --progress --database=database1.db tests/examples/diffs/diff1;" \
		"echo -n '  ' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt;" \
		"touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt;" \
		"rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt;" \
		"./precizer --update --progress --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command, result, 0));
	ASSERT(SUCCESS == get_file_content("templates/0011_002_3.txt", &pattern));
	ASSERT(SUCCESS == match_pattern(result->mem, pattern));

	free(pattern);
	pattern = NULL;
	del_char(&result);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};" \
				"rm database1.db;" \
				"rm -rf tests/examples/;" \
				"mv tests/examples_backup/ tests/examples/",0));

	RETURN_STATUS;
}

// Main test runner
Return test0011(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(readme_example_1_test,"Example 1 test…");
	TEST(readme_example_2_test,"Example 2 test…");

	RETURN_STATUS;
}
