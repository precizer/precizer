#include "sute.h"

#define OUTPUT_DRY_RUN_ARGUMENTS "--color=never --dry-run --database=0040.db tests/fixtures/diffs/diff1/2"
#define MACHINE_DRY_RUN_ARGUMENTS "--color=never --dry-run --database=0048.db 0048_empty"

/**
 * @brief Accept long and short output options with separate or attached values
 * @return Return status for complete dry-run output and empty stderr
 */
static Return test0048_1(void)
{
	INITTEST;
	const struct argument_test_case cases[] = {
		{"--output=full " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output=FULL " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"-OFULL " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output FuLl " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output full " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=MACHINE " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"-OMaChInE " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output MaChInE " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output machine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --output=full " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output=full --output=machine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"-O full " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"-Ofull " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"-O machine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"-Omachine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{"-Omachine --output=full " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0040_002.txt",STDERR_MUST_BE_EMPTY},
		{"--output=full -Omachine " OUTPUT_DRY_RUN_ARGUMENTS,COMPLETED,"templates/0048_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == run_and_match_templates(cases,"%COLOR_MODE%","never"));
	RETURN_STATUS;
}

/**
 * @brief Reject unknown, empty, and missing CLI output values
 * @return Return status for complete parsing diagnostics on both streams
 */
static Return test0048_2(void)
{
	INITTEST;
	const struct argument_test_case cases[] = {
		{"--output=invalid-value",FAILURE,"templates/0040_003.txt","templates/0048_001.txt"},
		{"--output invalid-value",FAILURE,"templates/0040_003.txt","templates/0048_001.txt"},
		{"--output=",FAILURE,"templates/0040_003.txt","templates/0048_002.txt"},
		{"--output ''",FAILURE,"templates/0040_003.txt","templates/0048_002.txt"},
		{"--output",FAILURE,"templates/0040_003.txt","templates/0048_002_1.txt"},
		{"-O invalid-value",FAILURE,"templates/0040_003.txt","templates/0048_001.txt"},
		{"-Oinvalid-value",FAILURE,"templates/0040_003.txt","templates/0048_001.txt"},
		{"-O ''",FAILURE,"templates/0040_003.txt","templates/0048_002.txt"},
		{"-O",FAILURE,"templates/0040_003.txt","templates/0048_002_2.txt"},
		{0}
	};

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	RETURN_STATUS;
}

/**
 * @brief Hide startup, traversal headings, and final statistics in ordinary machine runs
 *
 * An empty root produces no file events. TESTING=false exercises normal
 * initialization without the test-only trace emitted before argument parsing
 *
 * @return Return status for empty streams and dry-run database absence
 */
static Return test0048_3(void)
{
	INITTEST;
	const struct argument_test_case cases[] = {
		{"--output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--progress --output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --dry-run=with-checksums --color=never --database=0048.db 0048_empty",COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-sSNeVeR -oOMaChInE " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-vOMaChInE --silent " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--verbose --output=MaChInE " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-Omachine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=machine --verbose " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--verbose --output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-vOmachine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--silent --output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=machine --verbose --silent " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --verbose " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--verbose --silent --output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--silent --verbose --output=machine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=full -Omachine " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-Omachine --dry-run=with-checksums --color=never --database=0048.db 0048_empty",COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=full --silent --verbose " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"--output=full --verbose --silent " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-sv " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"-vs " MACHINE_DRY_RUN_ARGUMENTS,COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == create_directory("0048_empty"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	ASSERT(SUCCESS == expect_file_access_from_root(".","0048.db",F_OK,FILE_NOT_FOUND));
	call(set_environment_variable("TESTING","true"));
	call(delete_path_if_present("0048_empty"));
	RETURN_STATUS;
}

/**
 * @brief Apply output priorities before startup for explicit and default configuration files
 * @return Return status for empty output from configured dry runs
 */
static Return test0048_4(void)
{
	INITTEST;
	const char *config_templates[] = {
		"tests/templates/0048_004_1.conf",
		"tests/templates/0048_004_2.conf",
		"tests/templates/0048_004_3.conf",
		"tests/templates/0048_004_4.conf",
		"tests/templates/0048_004_5.conf",
		"tests/templates/0048_004_6.conf",
		"tests/templates/0048_004_7.conf",
		"tests/templates/0048_004_8.conf"
	};
	const struct argument_test_case cases[] = {
		{"--config=precizer.conf",COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{"",COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == create_directory("0048_empty"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	for(size_t i = 0; i < sizeof(config_templates) / sizeof(config_templates[0]) && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == copy_path(config_templates[i],"precizer.conf"));
		ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	}
	ASSERT(SUCCESS == expect_file_access_from_root(".","0048.db",F_OK,FILE_NOT_FOUND));
	call(set_environment_variable("TESTING","true"));
	call(delete_path_if_present("precizer.conf"));
	call(delete_path_if_present("0048_empty"));
	RETURN_STATUS;
}

/**
 * @brief Retain technical errors while machine output suppresses service messages
 *
 * Silent takes priority regardless of option order and suppresses logger
 * errors, while argp diagnostics remain on stderr
 *
 * @return Return status for complete diagnostics and successful return to full output
 */
static Return test0048_5(void)
{
	INITTEST;
	const struct argument_test_case cases[] = {
		{"--output=machine --color=never --output=invalid-value",FAILURE,"templates/0048_005_1.txt","templates/0048_001.txt"},
		{"--verbose -Omachine --color=never --output=invalid-value",FAILURE,"templates/0048_005_1.txt","templates/0048_001.txt"},
		{"-Omachine --verbose --color=never --output=invalid-value",FAILURE,"templates/0048_005_1.txt","templates/0048_001.txt"},
		{"-Omachine --verbose --silent --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"-Omachine --silent --verbose --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--silent -Omachine --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--verbose -Omachine --silent --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--verbose --silent -Omachine --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--silent -Omachine --verbose --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--silent --verbose -Omachine --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--output=full --silent --verbose --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"--output=full --verbose --silent --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"-sv --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"-vs --color=never --output=invalid-value",FAILURE,STDOUT_MUST_BE_EMPTY,"templates/0048_001.txt"},
		{"-Omachine --output=full --color=never --version",COMPLETED,"templates/0048_005_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	call(set_environment_variable("TESTING","true"));
	RETURN_STATUS;
}

/**
 * @brief Require the machine counting pass to leave traversal state untouched
 *
 * A permissive logger exposes unexpected service output. Nonzero counters and
 * timing detect an executed pass even when it has no configured roots
 *
 * @return SUCCESS when the requested progress pass performs no work
 */
static Return check_machine_counting_pass(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	Config *initial_config = config;
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	Config machine_config = {.output = OUTPUT_MACHINE,.progress = true};
	TraversalSummary summary = {.stats_only_pass = true,.count_files = 1U,.scan_elapsed_ns = 1LL};

	config = &machine_config;
	rational_logger_mode = REGULAR;
	run(file_list(&summary));
	ASSERT(summary.count_files == 1U);
	ASSERT(summary.scan_elapsed_ns == 1LL);
	ASSERT(machine_config.progress == true);

	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	provide(status);
}

/**
 * @brief Skip preliminary counting without diagnostics when machine progress is requested
 * @return Return status for an unchanged traversal state and empty output streams
 */
static Return test0048_6(void)
{
	INITTEST;
	ASSERT(SUCCESS == match_function_output(NULL,NULL,check_machine_counting_pass));
	RETURN_STATUS;
}

/**
 * @brief Preserve file hashing with machine progress from CLI and configuration
 *
 * Each run creates a fresh database and verifies its exact file list and final
 * SHA512 digest. Both output streams must remain free of service diagnostics
 *
 * @return Return status for complete scanning without progress output
 */
static Return test0048_7(void)
{
	INITTEST;

	/* Cover CLI option order, short options, and explicit and default configuration */
	const struct argument_test_case cases[] = {
		{"--progress --output=machine --database=0048.db tests/fixtures/diffs/diff1/2",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --progress --database=0048.db tests/fixtures/diffs/diff1/2",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{"-pOmachine --verbose --database=0048.db tests/fixtures/diffs/diff1/2",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{"--verbose --progress --output=machine --database=0048.db tests/fixtures/diffs/diff1/2",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{"--config=precizer.conf",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{"",COMPLETED,"templates/0048_007_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Locate the fixture file and define the complete expected database contents */
	const char *expected_paths[] = {"AAA/BBB/CZC/a.txt"};
	m_create(char,file_path,MEMORY_STRING);
	ASSERT(SUCCESS == construct_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt",file_path));

	/* Prepare the configuration used by both file-based scenarios */
	ASSERT(SUCCESS == copy_path("tests/templates/0048_007_3.conf","precizer.conf"));

	/* Disable test diagnostics so only machine file events remain visible */
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	/* Run each scenario separately so its database can be checked and removed */
	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		/* Pass one scenario and the required terminator to the shared runner */
		const struct argument_test_case single_case[] = {
			cases[i],
			{0}
		};

		/* Require the complete added record and empty stderr */
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));

		/* Verify that the main traversal stored exactly the expected file */
		ASSERT(SUCCESS == db_paths_match("0048.db",expected_paths,1));

		/* Compare the completed database checksum with an independent file hash */
		ASSERT(SUCCESS == db_final_sha512_matches_file("0048.db",expected_paths[0],m_text(file_path)));

		/* Remove the database so the next scenario starts with a fresh scan */
		call(delete_path_if_present("0048.db"));
	}

	/* Restore test diagnostics and release the temporary configuration and path */
	call(set_environment_variable("TESTING","true"));
	call(delete_path_if_present("precizer.conf"));
	call(m_del(file_path));
	RETURN_STATUS;
}

/**
 * @brief Report new ordinary and empty files as complete machine records
 *
 * Plain and colored templates require one record per file, with styling limited
 * to the action word. Real scans verify database contents, while both dry-run
 * variants must report the same events without creating a database file
 *
 * @return Return status for added events, output controls, and database contents
 */
static Return test0048_8(void)
{
	INITTEST;

	/* Cover plain and colored records, retained verbose settings, and suppression */
	const struct argument_test_case cases[] = {
		{"--output=machine --color=never --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --color=always --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3_1.txt",STDERR_MUST_BE_EMPTY},
		{"--verbose --progress --output=machine --color=never --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --quiet-ignored --color=never --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --silent --database=0048.db 0048_added",COMPLETED,STDOUT_MUST_BE_EMPTY,STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Both dry-run variants must retain the same machine event format */
	const struct argument_test_case dry_run_cases[] = {
		{"--output=machine --dry-run --color=never --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --dry-run=with-checksums --progress --color=never --database=0048.db 0048_added",COMPLETED,"templates/0048_008_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Create empty and nonempty files with deterministic traversal order */
	ASSERT(SUCCESS == create_directory("0048_added"));
	ASSERT(SUCCESS == write_string_to_file("","0048_added/001.txt",FILE_WRITE_REPLACE));
	ASSERT(SUCCESS == write_string_to_file("content","0048_added/002.txt",FILE_WRITE_REPLACE));
	const char *expected_paths[] = {"001.txt","002.txt"};
	m_create(char,file_path,MEMORY_STRING);
	ASSERT(SUCCESS == construct_path("0048_added/002.txt",file_path));

	/* Capture normal startup without test diagnostics */
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	/* Simulated scans must leave no database on disk */
	ASSERT(SUCCESS == run_and_match_templates(dry_run_cases,NULL,NULL));
	ASSERT(SUCCESS == expect_file_access_from_root(".","0048.db",F_OK,FILE_NOT_FOUND));

	/* Verify each real scan against a fresh database */
	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		/* Compare both complete output streams for this scenario */
		const struct argument_test_case single_case[] = {cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));

		/* Require both file records and the completed checksum of the nonempty file */
		ASSERT(SUCCESS == db_paths_match("0048.db",expected_paths,2));
		ASSERT(SUCCESS == db_final_sha512_matches_file("0048.db",expected_paths[1],m_text(file_path)));

		/* Prevent an existing database from affecting the next creation scenario */
		call(delete_path_if_present("0048.db"));
	}

	/* Restore test diagnostics and remove the isolated files */
	call(set_environment_variable("TESTING","true"));
	call(delete_path_if_present("0048_added"));
	call(m_del(file_path));
	RETURN_STATUS;
}

/**
 * @brief Exercise output format selection through application startup
 * @return Return status for format selection, logger filtering, and parsing diagnostics
 */
Return test0048(void)
{
	INITTEST;
	TEST(test0048_1,"Long and short output options accept repeated selections");
	TEST(test0048_2,"CLI output formats reject unknown, empty, and missing values");
	TEST(test0048_3,"Machine CLI output hides service messages during dry runs");
	TEST(test0048_4,"Explicit and default configuration select output priorities before startup");
	TEST(test0048_5,"Machine errors honor logger priorities and full output can be restored");
	TEST(test0048_6,"Machine progress skips preliminary counting without changing traversal state");
	TEST(test0048_7,"Machine progress preserves scanning and checksums through CLI and configuration");
	TEST(test0048_8,"Machine added records distinguish ordinary and empty files with keyword-only color");
	RETURN_STATUS;
}
