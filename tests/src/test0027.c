#include "sute.h"

/**
 * @brief Detect changes to files protected by repeated --lock-checksum rules
 *
 * CLI and configuration updates start from the same saved database and
 * modified files, then check the same full output and warning status.
 * Files outside the protected paths continue to update normally
 */
static Return test0027_1(void)
{
	INITTEST;

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff2"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case initial_cases[] = {
		{"--database=lock.db --lock-checksum=\"^diff1/1/.*\" "
		 "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs",COMPLETED,"templates/0027_001_1.txt",STDERR_MUST_BE_EMPTY},
		{"--update --check-level=QUICK --database=lock.db --lock-checksum=\"^diff1/1/.*\" "
		 "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs",COMPLETED,"templates/0027_001_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(initial_cases,NULL,NULL));

	ASSERT(SUCCESS == add_string_to("corrupted","tests/fixtures/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt"));
	ASSERT(SUCCESS == add_string_to("corrupted","tests/fixtures/diffs/diff2/path1/AAA/BCB/CCC/b.txt"));
	ASSERT(SUCCESS == add_string_to("changed","tests/fixtures/diffs/diff2/3/AAA/BBB/CCC/a.txt"));

	ASSERT(SUCCESS == copy_path("lock.db","lock1.db"));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff2/2/AAA/BBB/CZC/a.txt",999));

	const struct argument_test_case update_cases[] = {
		{"--progress --update --check-level=QUICK --database=lock.db "
		 "--lock-checksum=\"^diff1/1/.*\" "
		 "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs",WARNING,"templates/0027_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(update_cases,NULL,NULL));

	/* Restore the database for the configuration run without repeating file changes.
	   Keep lock1.db unchanged for the subsequent run without testing diagnostics */
	ASSERT(SUCCESS == copy_path("lock1.db","lock.db"));
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0027_001_3.conf",WARNING,"templates/0027_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const struct argument_test_case non_testing_cases[] = {
		{"--progress --update --check-level=QUICK --database=lock1.db "
		 "--lock-checksum=\"^diff1/1/.*\" "
		 "--lock-checksum=\"^diff2/path1/.*\" tests/fixtures/diffs",WARNING,"templates/0027_001_4.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(non_testing_cases,NULL,NULL));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("lock.db"));
	ASSERT(SUCCESS == delete_path("lock1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff2"));

	RETURN_STATUS;
}

Return test0027(void)
{
	INITTEST;

	TEST(test0027_1,"Attempt to modify files protected by the --lock-checksum");

	RETURN_STATUS;
}
