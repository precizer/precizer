#include "sute.h"

/**
 * Runs precizer with the given arguments and validates output with optional
 * stdout/stderr regex templates.
 */
static Return assert_compare_output(
	const char *arguments,
	const int  expected_return_code,
	const char *stdout_pattern_file,
	const char *stderr_pattern_file)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	if(arguments == NULL)
	{
		status = FAILURE;
	}

	unsigned int capture_policy = ALLOW_BOTH;

	if(expected_return_code != COMPLETED || stderr_pattern_file != NULL)
	{
		capture_policy = STDERR_ALLOW;
	}

	run(runit(arguments,stdout_result,stderr_result,expected_return_code,capture_policy));

	if(stdout_pattern_file != NULL)
	{
		run(get_file_content(stdout_pattern_file,stdout_pattern));
		run(match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));
	}

	if(stderr_pattern_file != NULL)
	{
		run(get_file_content(stderr_pattern_file,stderr_pattern));
		run(match_pattern(stderr_result,stderr_pattern,stderr_pattern_file));
	}

	m_del(stderr_pattern);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	deliver(status);
}

/**
 * Prepares two databases with known differences for compare-filter tests.
 */
static Return prepare_compare_filter_differences_fixture(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,result,MEMORY_STRING);

	run(prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	run(set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	run(delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt"));
	run(add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));
	run(replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	m_del(result);

	deliver(status);
}

/**
 * Prepares two databases where only one existence side differs:
 * database2 contains one extra path compared to database1.
 */
static Return prepare_compare_filter_one_sided_fixture(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,result,MEMORY_STRING);

	run(prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	run(set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	run(replase_to_string("ONLY_DB2_FILE","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/only_db2.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	m_del(result);

	deliver(status);
}

/**
 * Cleans temporary files created by the "differences" fixture.
 */
static Return cleanup_compare_filter_differences_fixture(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	run(delete_path("database1.db"));
	run(delete_path("database2.db"));
	run(restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	deliver(status);
}

/**
 * Prepares two equal databases for compare-filter tests.
 */
static Return prepare_compare_filter_equal_fixture(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,result,MEMORY_STRING);

	run(set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	run(runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if((SUCCESS & status) && result->length != 0)
	{
		status = FAILURE;
	}

	m_del(result);

	deliver(status);
}

/**
 * Cleans temporary files created by the "equal" fixture.
 */
static Return cleanup_compare_filter_equal_fixture(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	run(delete_path("database1.db"));
	run(delete_path("database2.db"));

	deliver(status);
}

/**
 * @brief Compare removed, modified, and added files through CLI and configuration
 *
 * Reuse the prepared databases for normal comparison and dry-run with checksums.
 * The complete output must retain the dry-run mode without traversal or hashing
 *
 * @return Return status for equivalent comparison output from both settings sources
 */
static Return test0028_1(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt")); // Remove
	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt")); // Modify
	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt")); // New file

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--check-level=QUICK --compare database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0028_001.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Repeat the comparison through the default configuration on the same databases */
	ASSERT(SUCCESS == copy_path("tests/templates/0028_001.conf","precizer.conf"));
	const struct argument_test_case file_cases[] = {
		{"",COMPLETED,"templates/0028_001.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	call(delete_path_if_present("precizer.conf"));

	/* Comparison reads saved checksums even when dry-run requests file hashing */
	const struct argument_test_case dry_run_cases[] = {
		{"--check-level=QUICK --dry-run=with-checksums --compare database1.db database2.db",
		 COMPLETED,"templates/0028_001_1.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_001_1.conf",COMPLETED,"templates/0028_001_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --dry-run=with-checksums --compare database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --dry-run=with-checksums --compare database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(dry_run_cases,NULL,NULL));

	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is removed. It should be reflected as a change in one of the databases.
 */
static Return test0028_2(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt")); // Remove

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--check-level=QUICK --compare database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0028_002_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_002_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_002_1_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is added. It should be reflected as a change in one of the databases.
 */
static Return test0028_3(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt")); // New file

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--check-level=QUICK --compare database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0028_003_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is updated and its checksum should change
 */
static Return test0028_4(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt")); // Modify

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--check-level=QUICK --compare database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0028_004.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * Nothing changes. The databases should be equivalent
 */
static Return test0028_5(void)
{
	INITTEST;

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--check-level=QUICK --compare database1.db database2.db";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0028_005.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	RETURN_STATUS;
}

/**
 * @brief Check compare-filter categories, repetitions, and validation through CLI and configuration
 */
static Return test0028_6(void)
{
	INITTEST;

	SLOWTEST;

	const struct argument_test_case equal_cases[] = {
		// Valid combinations with --compare for equal databases
		{"--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_005.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_006_1.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=qUiCk --compare --compare-filter=CHECKSUM-MISMATCH database1.db database2.db",COMPLETED,"templates/0028_006_1.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare database1.db database2.db --compare-filter=checksum-mismatch",COMPLETED,"templates/0028_006_1.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_2.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=qUiCk --compare --compare-filter=FIRST-SOURCE database1.db database2.db",COMPLETED,"templates/0028_006_2.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_3.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=qUiCk --compare --compare-filter=SECOND-SOURCE database1.db database2.db",COMPLETED,"templates/0028_006_3.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_4.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_5.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_6.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_19.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=qUiCk --compare --compare-filter=CHECKSUM-MISMATCH database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=qUiCk --compare --compare-filter=CHECKSUM-MISMATCH database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db --compare-filter=checksum-mismatch",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db --compare-filter=checksum-mismatch",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=qUiCk --compare --compare-filter=FIRST-SOURCE database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=qUiCk --compare --compare-filter=FIRST-SOURCE database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=qUiCk --compare --compare-filter=SECOND-SOURCE database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=qUiCk --compare --compare-filter=SECOND-SOURCE database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	const struct argument_test_case one_sided_cases[] = {
		// Regression: one-sided filter must not claim full identity if opposite side has differences
		{"--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_7.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_8.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare database2.db database1.db --compare-filter=first-source",COMPLETED,"templates/0028_006_17.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare database2.db database1.db --compare-filter=second-source",COMPLETED,"templates/0028_006_18.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_8_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_8_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare database2.db database1.db --compare-filter=first-source",COMPLETED,"templates/0028_006_17_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database2.db database1.db --compare-filter=first-source",COMPLETED,"templates/0028_006_17_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare database2.db database1.db --compare-filter=second-source",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database2.db database1.db --compare-filter=second-source",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	const struct argument_test_case differences_cases[] = {
		// Valid combinations with --compare for databases with all difference categories
		{"--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_006_21.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_006_21.conf",COMPLETED,"templates/0028_006_21.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_9.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_006_9.conf",COMPLETED,"templates/0028_006_9.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_10.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_006_10.conf",COMPLETED,"templates/0028_006_10.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_11.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_12.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_13.txt",STDERR_MUST_BE_EMPTY},
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_20.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_006_20.conf",COMPLETED,"templates/0028_006_20.txt",STDERR_MUST_BE_EMPTY},

		// Repeating a category must not duplicate its report or enable other categories
		{"--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_006_21.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_006_22.conf",COMPLETED,"templates/0028_006_21.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_002_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_002_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_11_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_006_11_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_12_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_12_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_13_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_006_13_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --compare-filter=checksum-mismatch --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		/* Reversing operands reverses the added and deleted records */
		{"--output=compact --check-level=QUICK --compare database2.db database1.db",COMPLETED,"templates/0028_006_23_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --silent --check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --silent --check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_002_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --silent --check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact --silent --check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --check-level=QUICK --compare database2.db database1.db",COMPLETED,"templates/0028_006_23_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --check-level=QUICK --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_002_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --check-level=QUICK --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --check-level=QUICK --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	const struct argument_test_case invalid_cases[] = {
		// Invalid combinations: --compare-filter=value without --compare
		{"--compare-filter=checksum-mismatch database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--config=tests/templates/0028_006_14.conf",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=first-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=second-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=second-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=first-source --compare-filter=second-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source --compare-filter=second-source database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},

		// Invalid combinations: --compare-filter without argument
		{"--compare database1.db database2.db --compare-filter",FAILURE,NULL,"templates/0028_006_16.txt"},
		{"--compare-filter",FAILURE,NULL,"templates/0028_006_16.txt"},
		{0}
	};

	ASSERT(SUCCESS & prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == run_and_match_templates(equal_cases,NULL,NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_equal_fixture());

	ASSERT(SUCCESS & prepare_compare_filter_one_sided_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == run_and_match_templates(one_sided_cases,NULL,NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_differences_fixture());

	ASSERT(SUCCESS & prepare_compare_filter_differences_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == run_and_match_templates(differences_cases,NULL,NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_differences_fixture());

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == run_and_match_templates(invalid_cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * @brief Reject empty and unsupported compare-filter values with or without comparison
 *
 * Both settings sources stop at the first invalid value. Later valid or invalid
 * entries must neither hide that error nor produce additional diagnostics
 */
static Return test0028_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--compare --compare-filter=invalid-value database1.db database2.db",FAILURE,"templates/0028_007_3.txt","templates/0028_007_1.txt"},
		{"--config=tests/templates/0028_007_1.conf",FAILURE,"templates/0028_007_4.txt","templates/0028_007_2.txt"},
		{"--compare --compare-filter= database1.db database2.db",FAILURE,"templates/0028_007_3.txt","templates/0028_007_5.txt"},
		{"--config=tests/templates/0028_007_2.conf",FAILURE,"templates/0028_007_4.txt","templates/0028_007_6.txt"},

		// Value errors precede the check requiring comparison mode
		{"--compare-filter=invalid-value database1.db database2.db",FAILURE,"templates/0028_007_3.txt","templates/0028_007_1.txt"},
		{"--config=tests/templates/0028_007_3.conf",FAILURE,"templates/0028_007_4.txt","templates/0028_007_7.txt"},
		{"--compare-filter= database1.db database2.db",FAILURE,"templates/0028_007_3.txt","templates/0028_007_5.txt"},
		{"--config=tests/templates/0028_007_4.conf",FAILURE,"templates/0028_007_4.txt","templates/0028_007_8.txt"},

		// Only the first invalid value is reported, regardless of subsequent entries
		{"--compare --compare-filter=invalid-value --compare-filter=first-source --compare-filter=later-invalid-value database1.db database2.db",FAILURE,"templates/0028_007_3.txt","templates/0028_007_1.txt"},
		{"--config=tests/templates/0028_007_5.conf",FAILURE,"templates/0028_007_4.txt","templates/0028_007_2.txt"},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 * NULL vs non-NULL SHA512 rows must be reported as checksum mismatches
 */
static Return test0028_8(void)
{
	INITTEST;

	ASSERT(SUCCESS & prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == db_set_sha512_to_null("database2.db","1/AAA/ZAW/D/e/f/b_file.txt"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_000.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_equal_fixture());

	RETURN_STATUS;
}

/**
 * NULL SHA512 produced by fixture changes must be reported as a mismatch
 */
static Return test0028_9(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == truncate_file_to_zero_size("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_000.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database1.db database2.db",COMPLETED,"templates/0028_004_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	m_del(result);

	RETURN_STATUS;
}

/**
 * Compare mode must support attached database paths containing apostrophes
 */
static Return test0028_10(void)
{
	INITTEST;

	ASSERT(SUCCESS & prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == move_path("database1.db","database'1.db"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--check-level=QUICK --compare database\\'1.db database2.db",COMPLETED,"templates/0028_010_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare database\\'1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare database\\'1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* The apostrophe is literal in the configuration value */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0028_010_1.conf",COMPLETED,"templates/0028_010_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	/* Give one stored path punctuation that must remain literal in compare records */
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	ASSERT(SUCCESS == open_db_from_tmpdir("database'1.db",SQLITE_OPEN_READWRITE,&db));
	ASSERT(SQLITE_OK == sqlite3_prepare_v2(db,
		"UPDATE files SET relative_path = ?1 WHERE relative_path = '1/AAA/ZAW/D/e/f/b_file.txt';",-1,&stmt,NULL));
	ASSERT(SQLITE_OK == sqlite3_bind_text(stmt,1,"odd [name] 'quote' \\ path.txt",-1,SQLITE_STATIC));
	ASSERT(SQLITE_DONE == sqlite3_step(stmt));
	ASSERT(sqlite3_changes(db) == 1);
	(void)sqlite3_finalize(stmt);
	(void)sqlite3_close(db);
	const struct argument_test_case path_cases[] = {
		{"--output=compact --check-level=QUICK --compare database\\'1.db database2.db",COMPLETED,"templates/0028_010_2_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --check-level=QUICK --compare database\\'1.db database2.db",COMPLETED,"templates/0028_010_2_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(path_cases,NULL,NULL));

	ASSERT(SUCCESS == delete_path("database'1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	RETURN_STATUS;
}

/**
 * Silent compare mode should print only compare results
 */
static Return test0028_11(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	INITTEST;

	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS & prepare_compare_filter_differences_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS & assert_compare_output("--check-level=QUICK --silent --compare database1.db database2.db",COMPLETED,"templates/0028_011_1.txt",NULL));

	/* Repeat the silent comparison without rebuilding either database */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0028_011_1.conf",COMPLETED,"templates/0028_011_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));

	ASSERT(SUCCESS & assert_compare_output("--check-level=QUICK --silent --compare --compare-filter=first-source database1.db database2.db",COMPLETED,"templates/0028_011_2.txt",NULL));
	ASSERT(SUCCESS & assert_compare_output("--check-level=QUICK --silent --compare --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_011_5.txt",NULL));
	ASSERT(SUCCESS & assert_compare_output("--check-level=QUICK --silent --compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_011_3.txt",NULL));
	ASSERT(SUCCESS & assert_compare_output("--check-level=QUICK --silent --compare --compare-filter=first-source --compare-filter=second-source database1.db database2.db",COMPLETED,"templates/0028_011_4.txt",NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_differences_fixture());

	ASSERT(SUCCESS & prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == runit("--check-level=QUICK --silent --compare database1.db database2.db",result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(result->length == 0);

	ASSERT(SUCCESS & cleanup_compare_filter_equal_fixture());

	m_del(result);

	RETURN_STATUS;
}

/**
 * Compare mode must apply --ignore and --include to the reported compare scope
 *
 * Hidden paths are treated as out of scope for category listings, category
 * summaries, and final equality messages. Paths restored with --include become
 * visible again even when they also match an --ignore pattern
 */
static Return test0028_12(void)
{
	INITTEST;

	ASSERT(SUCCESS & prepare_compare_filter_differences_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		// All three raw differences are filtered out, so the filtered compare scope is fully identical
		{"--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" database1.db database2.db",COMPLETED,"templates/0028_012_1.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_012_1.conf",COMPLETED,"templates/0028_012_1.txt",STDERR_MUST_BE_EMPTY},

		// Restore only the first-source path a.txt; the hidden checksum mismatch stays outside the reported scope
		{"--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_012_2.txt",STDERR_MUST_BE_EMPTY},

		// Keep only the first-source path a.txt visible by filtering out the checksum mismatch and the opposite-side path
		{"--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_012_3.txt",STDERR_MUST_BE_EMPTY},

		// Hide only a.txt so the remaining reported scope still contains the opposite-side path and the checksum mismatch
		{"--check-level=QUICK --compare --ignore=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_012_4.txt",STDERR_MUST_BE_EMPTY},

		// Ignore everything, then restore only b.txt; the checksum mismatch remains intentionally out of scope
		{"--check-level=QUICK --compare --ignore=\"^.*$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_012_5.txt",STDERR_MUST_BE_EMPTY},

		// Ignore the whole 2/ subtree, then restore both existence-side differences while the 1/ checksum mismatch remains visible
		{"--check-level=QUICK --compare --ignore=\"^2/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_012_6.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0028_012_6.conf",COMPLETED,"templates/0028_012_6.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the same comparison without headings or summary diagnostics */
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" database1.db database2.db",COMPLETED,"templates/0028_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_002_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_002_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_002_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" --ignore=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_002_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_006_12_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^2/AAA/BBB/CZC/a\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_006_12_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^.*$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_003_1_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^.*$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_003_1_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=compact " "--check-level=QUICK --compare --ignore=\"^2/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--check-level=QUICK --compare --ignore=\"^2/.*$\" --include=\"^2/AAA/BBB/CZC/a\\.txt$\" --include=\"^2/AAA/BBB/CZC/b\\.txt$\" database1.db database2.db",COMPLETED,"templates/0028_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS & cleanup_compare_filter_differences_fixture());

	RETURN_STATUS;
}

/**
 * @brief Reject comparison with one or three database paths from either settings source
 * @return Return status for matching CLI and configuration diagnostics
 */
static Return test0028_13(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const struct argument_test_case cases[] = {
		{"--compare first.db",FAILURE,"templates/0028_006_14.txt","templates/0003_011.txt"},
		{"--config=tests/templates/0028_013_1.conf",FAILURE,"templates/0028_006_14.txt","templates/0003_011.txt"},
		{"--compare first.db second.db third.db",FAILURE,"templates/0028_006_14.txt","templates/0028_013_1.txt"},
		{"--config=tests/templates/0028_013_2.conf",FAILURE,"templates/0028_006_14.txt","templates/0028_013_1.txt"},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	RETURN_STATUS;
}

/**
 *
 * Testing the --compare mode across different types of responses
 *
 */
Return test0028(void)
{
	INITTEST;

	TEST(test0028_1,"One file is removed, updated, and added at a time");
	TEST(test0028_2,"One file is removed. It should be reflected as a change in one of the databases");
	TEST(test0028_3,"One file is added. It should be reflected as a change in one of the databases");
	TEST(test0028_4,"One file is updated and its checksum should change");
	TEST(test0028_5,"Nothing changes. The databases should be equivalent");
	TEST(test0028_6,"All supported --compare-filter combinations should behave as expected");
	TEST(test0028_7,"Invalid --compare-filter value should fail with an argument parsing error");
	TEST(test0028_8,"NULL and non-NULL SHA512 values should be reported as mismatches");
	TEST(test0028_9,"NULL SHA512 created from fixture changes should be reported as a mismatch");
	TEST(test0028_10,"Compare mode should work with database names containing apostrophes");
	TEST(test0028_11,"Silent compare mode should print only compare results");
	TEST(test0028_12,"Compare mode should apply --ignore and --include to the reported comparison scope, including summaries and equality messages");
	TEST(test0028_13,"Comparison requires exactly two database paths through CLI and configuration");

	RETURN_STATUS;
}
