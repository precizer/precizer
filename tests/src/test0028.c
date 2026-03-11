#include "sute.h"

/**
 * Runs precizer with the given arguments and validates output with optional
 * stdout/stderr regex templates.
 */
static Return assert_compare_output(
	const char *arguments,
	const int expected_return_code,
	const char *stdout_pattern_file,
	const char *stderr_pattern_file)
{
	INITTEST;

	create(char,stdout_result);
	create(char,stderr_result);
	create(char,stdout_pattern);
	create(char,stderr_pattern);

	ASSERT(arguments != NULL);

	unsigned int capture_policy = ALLOW_BOTH;

	if(expected_return_code != COMPLETED || stderr_pattern_file != NULL)
	{
		capture_policy = STDERR_ALLOW;
	}

	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,expected_return_code,capture_policy));

	if(stdout_pattern_file != NULL)
	{
		ASSERT(SUCCESS == get_file_content(stdout_pattern_file,stdout_pattern));
		ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));
	}

	if(stderr_pattern_file != NULL)
	{
		ASSERT(SUCCESS == get_file_content(stderr_pattern_file,stderr_pattern));
		ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern,stderr_pattern_file));
	}

	del(stderr_pattern);
	del(stdout_pattern);
	del(stderr_result);
	del(stdout_result);

	deliver(status);
}

/**
 * Prepares two databases with known differences for compare-filter tests.
 */
static Return prepare_compare_filter_differences_fixture(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt"));
	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));
	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	del(result);

	deliver(status);
}

/**
 * Prepares two databases where only one existence side differs:
 * database2 contains one extra path compared to database1.
 */
static Return prepare_compare_filter_one_sided_fixture(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == replase_to_string("ONLY_DB2_FILE","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/only_db2.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	del(result);

	deliver(status);
}

/**
 * Cleans temporary files created by the "differences" fixture.
 */
static Return cleanup_compare_filter_differences_fixture(void)
{
	INITTEST;

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	deliver(status);
}

/**
 * Prepares two equal databases for compare-filter tests.
 */
static Return prepare_compare_filter_equal_fixture(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	del(result);

	deliver(status);
}

/**
 * Cleans temporary files created by the "equal" fixture.
 */
static Return cleanup_compare_filter_equal_fixture(void)
{
	INITTEST;

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	deliver(status);
}

/**
 * One file is removed, updated, and added at a time
 */
static Return test0028_1(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

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

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is removed. It should be reflected as a change in one of the databases.
 */
static Return test0028_2(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt")); // Remove

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is added. It should be reflected as a change in one of the databases.
 */
static Return test0028_3(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt")); // New file

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * One file is updated and its checksum should change
 */
static Return test0028_4(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	// Preparation for tests
	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt")); // Modify

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_004.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * Nothing changes. The databases should be equivalent
 */
static Return test0028_5(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_005.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	RETURN_STATUS;
}

/**
 * Compare-filter combinations in compare mode and argument validation.
 */
static Return test0028_6(void)
{
	INITTEST;

	struct compare_filter_case {
		const char *arguments;
		int expected_return_code;
		const char *stdout_pattern_file;
		const char *stderr_pattern_file;
	};

	const struct compare_filter_case equal_cases[] = {
		// Valid combinations with --compare for equal databases
		{"--compare database1.db database2.db",COMPLETED,"templates/0028_005.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_006_1.txt",NULL},
		{"--compare database1.db database2.db --compare-filter=checksum-mismatch",COMPLETED,"templates/0028_006_1.txt",NULL},
		{"--compare --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_006_2.txt",NULL},
		{"--compare --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_3.txt",NULL},
		{"--compare --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_4.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_006_5.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_6.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_19.txt",NULL}
	};

	const struct compare_filter_case one_sided_cases[] = {
		// Regression: one-sided filter must not claim full identity if opposite side has differences
		{"--compare --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_006_7.txt",NULL},
		{"--compare --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_8.txt",NULL},
		{"--compare database2.db database1.db --compare-filter=first-source-only",COMPLETED,"templates/0028_006_17.txt",NULL},
		{"--compare database2.db database1.db --compare-filter=second-source-only",COMPLETED,"templates/0028_006_18.txt",NULL}
	};

	const struct compare_filter_case differences_cases[] = {
		// Valid combinations with --compare for databases with all difference categories
		{"--compare database1.db database2.db",COMPLETED,"templates/0028_001.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_006_21.txt",NULL},
		{"--compare --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_006_9.txt",NULL},
		{"--compare --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_10.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_006_11.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_12.txt",NULL},
		{"--compare --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_13.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006_20.txt",NULL}
	};

	const struct compare_filter_case invalid_cases[] = {
		// Invalid combinations: --compare-filter=value without --compare
		{"--compare-filter=checksum-mismatch database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=first-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_006_14.txt","templates/0028_006_15.txt"},

		// Invalid combinations: --compare-filter without argument
		{"--compare database1.db database2.db --compare-filter",FAILURE,NULL,"templates/0028_006_16.txt"},
		{"--compare-filter",FAILURE,NULL,"templates/0028_006_16.txt"}
	};

	ASSERT(SUCCESS == prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	for(size_t i = 0; (i < sizeof(equal_cases) / sizeof(equal_cases[0])) && (SUCCESS == status); i++)
	{
		ASSERT(SUCCESS == assert_compare_output(
			equal_cases[i].arguments,
			equal_cases[i].expected_return_code,
			equal_cases[i].stdout_pattern_file,
			equal_cases[i].stderr_pattern_file));
	}

	ASSERT(SUCCESS == cleanup_compare_filter_equal_fixture());

	ASSERT(SUCCESS == prepare_compare_filter_one_sided_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	for(size_t i = 0; (i < sizeof(one_sided_cases) / sizeof(one_sided_cases[0])) && (SUCCESS == status); i++)
	{
		ASSERT(SUCCESS == assert_compare_output(
			one_sided_cases[i].arguments,
			one_sided_cases[i].expected_return_code,
			one_sided_cases[i].stdout_pattern_file,
			one_sided_cases[i].stderr_pattern_file));
	}

	ASSERT(SUCCESS == cleanup_compare_filter_differences_fixture());

	ASSERT(SUCCESS == prepare_compare_filter_differences_fixture());
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	for(size_t i = 0; (i < sizeof(differences_cases) / sizeof(differences_cases[0])) && (SUCCESS == status); i++)
	{
		ASSERT(SUCCESS == assert_compare_output(
			differences_cases[i].arguments,
			differences_cases[i].expected_return_code,
			differences_cases[i].stdout_pattern_file,
			differences_cases[i].stderr_pattern_file));
	}

	ASSERT(SUCCESS == cleanup_compare_filter_differences_fixture());

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	for(size_t i = 0; (i < sizeof(invalid_cases) / sizeof(invalid_cases[0])) && (SUCCESS == status); i++)
	{
		ASSERT(SUCCESS == assert_compare_output(
			invalid_cases[i].arguments,
			invalid_cases[i].expected_return_code,
			invalid_cases[i].stdout_pattern_file,
			invalid_cases[i].stderr_pattern_file));
	}

	RETURN_STATUS;
}

/**
 * Invalid --compare-filter value should fail argument parsing
 */
static Return test0028_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--compare --compare-filter=invalid-value database1.db database2.db";

	ASSERT(SUCCESS == assert_compare_output(arguments,FAILURE,NULL,"templates/0028_007_1.txt"));

	RETURN_STATUS;
}

/**
 * NULL vs non-NULL SHA512 rows must be reported as checksum mismatches
 */
static Return test0028_8(void)
{
	INITTEST;

	ASSERT(SUCCESS == prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == db_set_sha512_to_null("database2.db","1/AAA/ZAW/D/e/f/b_file.txt"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == assert_compare_output("--compare database1.db database2.db",COMPLETED,"templates/0028_000.txt",NULL));

	ASSERT(SUCCESS == cleanup_compare_filter_equal_fixture());

	RETURN_STATUS;
}

/**
 * NULL SHA512 produced by fixture changes must be reported as a mismatch
 */
static Return test0028_9(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == truncate_file_to_zero_size("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));

	arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == assert_compare_output(
		"--compare database1.db database2.db",
		COMPLETED,
		"templates/0028_000.txt",
		NULL));

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(result);

	RETURN_STATUS;
}

/**
 * Compare mode must support attached database paths containing apostrophes
 */
static Return test0028_10(void)
{
	INITTEST;

	ASSERT(SUCCESS == prepare_compare_filter_equal_fixture());
	ASSERT(SUCCESS == move_path("database1.db","database'1.db"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == assert_compare_output(
		"--compare database\\'1.db database2.db",
		COMPLETED,
		"templates/0028_010_1.txt",
		NULL));

	ASSERT(SUCCESS == delete_path("database'1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

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

	TEST(test0028_1,"One file is removed, updated, and added at a time…");
	TEST(test0028_2,"One file is removed. It should be reflected as a change in one of the databases…");
	TEST(test0028_3,"One file is added. It should be reflected as a change in one of the databases…");
	TEST(test0028_4,"One file is updated and its checksum should change…");
	TEST(test0028_5,"Nothing changes. The databases should be equivalent…");
	TEST(test0028_6,"All supported --compare-filter combinations should behave as expected…");
	TEST(test0028_7,"Invalid --compare-filter value should fail with an argument parsing error…");
	TEST(test0028_8,"NULL and non-NULL SHA512 values should be reported as mismatches…");
	TEST(test0028_9,"NULL SHA512 created from fixture changes should be reported as a mismatch…");
	TEST(test0028_10,"Compare mode should work with database names containing apostrophes…");

	RETURN_STATUS;
}
