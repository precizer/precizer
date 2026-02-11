#include "sute.h"

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

	return(status);
}

static Return prepare_compare_filter_differences_fixture(void)
{
	INITTEST;

	create(char,result);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
	}

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;"
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
	}

	del(result);

	RETURN_STATUS;
}

static Return cleanup_compare_filter_differences_fixture(void)
{
	INITTEST;

	const char *command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db && "
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

static Return prepare_compare_filter_equal_fixture(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
	}

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
	}

	del(result);

	RETURN_STATUS;
}

static Return cleanup_compare_filter_equal_fixture(void)
{
	INITTEST;

	const char *command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is removed, updated, and added at a time
 */
static Return test0028_1_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" // Remove
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Modify
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db && "
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is removed. It should be reflected as a change in one of the databases.
 */
static Return test0028_2_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;"; // Remove

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db && "
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is added. It should be reflected as a change in one of the databases.
 */
static Return test0028_3_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db && "
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * One file is updated and its checksum should change
 */
static Return test0028_4_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	command = "cd ${TMPDIR};"
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"; // Modify

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--silent --database=database2.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	if(result->length > 0)
	{
		echo(STDERR,"STDOUT buffer is not empty. It contains characters: %zu\n",result->length);
		status = FAILURE;
		#if 0
		echo(STDOUT,"%s\n",getcstring(result));
		#endif
	}

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0028_004.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db && "
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Nothing changes. The databases should be equivalent
 */
static Return test0028_5_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	create(char,pattern);

	const char *arguments = "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--database=database2.db tests/examples/diffs/diff1";

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
	const char *command = "cd ${TMPDIR} && "
	        "rm database1.db database2.db;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Compare-filter combinations in compare mode and argument validation.
 */
static Return test0028_6_compare_filter_combinations_test(void)
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
		{"--compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_014.txt",NULL},
		{"--compare --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_015.txt",NULL},
		{"--compare --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_015.txt",NULL},
		{"--compare --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_015.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_016.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_016.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_005.txt",NULL}
	};

	const struct compare_filter_case differences_cases[] = {
		// Valid combinations with --compare for databases with all difference categories
		{"--compare database1.db database2.db",COMPLETED,"templates/0028_001.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch database1.db database2.db",COMPLETED,"templates/0028_004.txt",NULL},
		{"--compare --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_008.txt",NULL},
		{"--compare --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_009.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",COMPLETED,"templates/0028_010.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_011.txt",NULL},
		{"--compare --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_006.txt",NULL},
		{"--compare --compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",COMPLETED,"templates/0028_001.txt",NULL}
	};

	const struct compare_filter_case invalid_cases[] = {
		// Invalid combinations: --compare-filter=value without --compare
		{"--compare-filter=checksum-mismatch database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=first-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},
		{"--compare-filter=checksum-mismatch --compare-filter=first-source-only --compare-filter=second-source-only database1.db database2.db",FAILURE,"templates/0028_012_1.txt","templates/0028_012_2.txt"},

		// Invalid combinations: --compare-filter without argument
		{"--compare database1.db database2.db --compare-filter",FAILURE,NULL,"templates/0028_013.txt"},
		{"--compare-filter",FAILURE,NULL,"templates/0028_013.txt"}
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
static Return test0028_7_compare_filter_invalid_value_test(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--compare --compare-filter=invalid-value database1.db database2.db";

	ASSERT(SUCCESS == assert_compare_output(arguments,FAILURE,NULL,"templates/0028_007.txt"));

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

	TEST(test0028_1_test,"One file is removed, updated, and added at a time…");
	TEST(test0028_2_test,"One file is removed. It should be reflected as a change in one of the databases…");
	TEST(test0028_3_test,"One file is added. It should be reflected as a change in one of the databases…");
	TEST(test0028_4_test,"One file is updated and its checksum should change…");
	TEST(test0028_5_test,"Nothing changes. The databases should be equivalent…");
	TEST(test0028_6_compare_filter_combinations_test,"All supported --compare-filter combinations should behave as expected…");
	TEST(test0028_7_compare_filter_invalid_value_test,"Invalid --compare-filter value should fail with an argument parsing error…");

	RETURN_STATUS;
}
