#include "sute.h"

/**
 *
 * Testing database creation attempt in missing directory
 *
 */
Return test0020_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=nonexistent_directory/database1.db "
	        "tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,FAILURE,"templates/0020_001.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * @brief Reject updating a missing database through CLI and configuration
 *
 * Both runs use the same full output template and must fail without creating
 * the database
 *
 * @return Return status for equivalent update failures
 */
Return test0020_2(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=nonexistent_database1.db "
	        "tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,FAILURE,"templates/0020_002.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Repeat the failure through configuration while the database is still absent */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0020_002.conf",FAILURE,"templates/0020_002.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * @brief Reject database creation in a write-protected directory through CLI and configuration
 *
 * Both runs use the same directory permissions and full output template
 *
 * @return Return status for equivalent database creation failures
 */
Return test0020_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == create_directory("write_protected_directory"));
	ASSERT(SUCCESS == change_mode("write_protected_directory",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=write_protected_directory/database1.db "
	        "tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,FAILURE,"templates/0020_003.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Repeat the failure before restoring permissions on the database directory */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0020_003.conf",FAILURE,"templates/0020_003.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=write_protected_database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == change_mode("write_protected_database1.db",0000));

	arguments = "--compare write_protected_database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,FAILURE,"templates/0020_004.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=write_protected_database1.db "
	        "tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,FAILURE,"templates/0020_005.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == change_mode("write_protected_database1.db",0666));
	ASSERT(SUCCESS == delete_path("write_protected_database1.db"));

	RETURN_STATUS;
}

/**
 * @brief Reject changing the primary path without force through CLI and configuration
 *
 * Both runs use the same prepared database and full output template. The warning
 * leaves the database unchanged for the following test
 *
 * @return Return status for equivalent path mismatch warnings
 */
Return test0020_6(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--check-level=QUICK --update --database=database1.db tests/fixtures/diffs/diff2";

	const struct argument_test_case cases[] = {
		{arguments,WARNING,"templates/0020_006.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Repeat the warning with force disabled in the configuration file */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0020_006.conf",WARNING,"templates/0020_006.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * @brief Replace the primary path with force through CLI and configuration
 *
 * Both updates start from the same saved database and use the same full output
 * template. File timestamps are prepared once before either update
 *
 * @return Return status for equivalent forced path replacements
 */
Return test0020_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Match mtime for files with identical content in diff1 and diff2.
	// Setting the timestamp updates ctime, so --update --force can refresh
	// their metadata without rehashing when --watch-timestamps is not used
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/1/AAA/BCB/CCC/a.txt",0));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/A/b/c/a_file.txt",0));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt","tests/fixtures/diffs/diff2/1/AAA/ZAW/D/e/f/b_file.txt",0));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt","tests/fixtures/diffs/diff2/path1/AAA/BCB/CCC/a.txt",0));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path1/AAA/ZAW/A/b/c/a_file.txt",0));
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns("tests/fixtures/diffs/diff1/path2/AAA/ZAW/A/b/c/a_file.txt","tests/fixtures/diffs/diff2/path2/AAA/ZAW/A/b/c/a_file.txt",0));

	/* Save the database before either update replaces its stored paths and records */
	ASSERT(SUCCESS == copy_path("database1.db","0020_007.db"));

	const char *arguments = "--check-level=QUICK --update --force --database=database1.db "
	        "tests/fixtures/diffs/diff2";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0020_007.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	// Match the result against the pattern
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Restore only the database so configuration processes the same prepared files */
	ASSERT(SUCCESS == copy_path("0020_007.db","database1.db"));
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0020_007.conf",COMPLETED,"templates/0020_007.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	call(delete_path_if_present("0020_007.db"));

	ASSERT(SUCCESS == delete_path("database1.db"));

	RETURN_STATUS;
}

Return test0020(void)
{
	INITTEST;

	TEST(test0020_1,"DB creation in missing directory")
	TEST(test0020_2,"Attempt to open DB with --update when database is missing")
	TEST(test0020_3,"DB creation in write protected directory")
	TEST(test0020_4,"Attempt to open DB with write protected database file")
	TEST(test0020_5,"Attempt to update DB with --update when database file is write protected")
	TEST(test0020_6,"Attempt to change the primary path in the database")
	TEST(test0020_7,"Replace the primary path in the database")

	RETURN_STATUS;
}
