#include "sute.h"

/**
 * @brief Validate file-level ignore filtering through CLI and configuration
 *
 * Every scan starts without a database and checks complete output and exact
 * stored paths. CLI and configuration share a full template, followed by
 * compact and machine CLI runs. A second CLI/configuration pair checks the
 * TESTING diagnostics against one full template with the configuration path
 * allowed only at its expected position
 *
 * @return Return status for equivalent filtering and diagnostic output
 */
Return test0009_1(void)
{
	INITTEST;
	const char *db_filename = "database0009.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009.db "
	        "--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	/* Require exactly the six paths that do not match the ignore expression */
	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"omega_77xy__aa.bin",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	/* Compare equivalent full scans, then repeat the CLI operation in each compact format */
	const struct argument_test_case non_testing_cases[] = {
		{arguments,COMPLETED,"templates/0009_001_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the initial scan from an empty database with the same ignore rule */
		{"--config=tests/templates/0009_001_1.conf",COMPLETED,"templates/0009_001_1.txt",STDERR_MUST_BE_EMPTY},
		/* Compact output retains terminal styling for added and ignored records */
		{"--output=compact --database=database0009.db "
		 "--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
		 "tests/fixtures/ignore_include_cases/chaotic_filenames",COMPLETED,"templates/0009_001_2.txt",STDERR_MUST_BE_EMPTY},
		/* Machine output contains the same records without styling or service messages */
		{"--output=machine --database=database0009.db "
		 "--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
		 "tests/fixtures/ignore_include_cases/chaotic_filenames",COMPLETED,"templates/0009_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Remove the preceding database so every format performs an initial scan */
	for(size_t i = 0; non_testing_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == delete_path_if_present(db_filename));
		const struct argument_test_case single_case[] = {
			non_testing_cases[i],
			{0}
		};

		/* Check the complete output, empty stderr, and exact stored paths */
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	}

	/* Repeat the CLI/configuration pair with the same TESTING diagnostics enabled */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	const struct argument_test_case testing_cases[] = {
		{arguments,COMPLETED,"templates/0009_001_1_1.txt",STDERR_MUST_BE_EMPTY},
		{"--config=tests/templates/0009_001_1.conf",COMPLETED,"templates/0009_001_1_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Restore the absent-database state and repeat all output and path checks */
	for(size_t i = 0; testing_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == delete_path_if_present(db_filename));
		const struct argument_test_case single_case[] = {
			testing_cases[i],
			{0}
		};

		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	}

	/* Clean up the final database even if an earlier check failed */
	call(delete_path_if_present(db_filename));

	RETURN_STATUS;
}

/**
 * @brief Validate include exceptions to ignore rules through CLI and configuration
 *
 * CLI and configuration share a full template, followed by compact and machine
 * CLI runs. Every scan starts without a database and checks complete output
 * and exact stored paths, including the files restored by include
 *
 * @return Return status for equivalent include filtering
 */
static Return test0009_2(void)
{
	INITTEST;
	const char *db_filename = "database0009_2.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_2.db "
	        "--ignore=\"^(?:skip_|tmp_|zeta_|omega_).+\" "
	        "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|zeta_z1-9vv\\.dat)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	/* Require the ordinary tracked files and all three explicit include exceptions */
	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"skip_4xv7__m2.log",
		"tmp_qwe_90210.log",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	/* Compare equivalent full scans, then repeat the CLI operation in each compact format */
	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_002_1.txt",STDERR_MUST_BE_EMPTY},
		/* Repeat the scan with the same ignore and include rules from configuration */
		{"--config=tests/templates/0009_002_1.conf",COMPLETED,"templates/0009_002_1.txt",STDERR_MUST_BE_EMPTY},
		/* Compact output distinguishes include exceptions from ordinary additions */
		{"--output=compact --database=database0009_2.db "
		 "--ignore=\"^(?:skip_|tmp_|zeta_|omega_).+\" "
		 "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|zeta_z1-9vv\\.dat)$\" "
		 "tests/fixtures/ignore_include_cases/chaotic_filenames",COMPLETED,"templates/0009_002_2.txt",STDERR_MUST_BE_EMPTY},
		/* Machine output preserves the same classifications without terminal styling */
		{"--output=machine --database=database0009_2.db "
		 "--ignore=\"^(?:skip_|tmp_|zeta_|omega_).+\" "
		 "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|zeta_z1-9vv\\.dat)$\" "
		 "tests/fixtures/ignore_include_cases/chaotic_filenames",COMPLETED,"templates/0009_002_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	/* Remove the preceding database so every run exercises the same initial filtering */
	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == delete_path_if_present(db_filename));
		const struct argument_test_case single_case[] = {
			cases[i],
			{0}
		};

		/* Check the complete output, empty stderr, and exact stored paths */
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	}

	/* Clean up the final database even if an earlier check failed */
	call(delete_path_if_present(db_filename));

	RETURN_STATUS;
}

/**
 *
 * Validate whole-directory ignore with selective include
 *
 */
static Return test0009_3(void)
{
	INITTEST;
	const char *db_filename = "database0009_3.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_3.db "
	        "--ignore=\"^chaotic_filenames(?:/|$)\" "
	        "--include=\"^chaotic_filenames/(?:alpha_m0n9k2_zz\\.txt|hold_a1r9v-0pq\\.bak|keep_4xv7__m2\\.log|omega_77xy__aa\\.bin|xqwe_90210\\.md|zeta_z1-9vv\\.dat)$\" "
	        "tests/fixtures/ignore_include_cases";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_003.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	const char *expected_paths[] =
	{
		"chaotic_filenames/alpha_m0n9k2_zz.txt",
		"chaotic_filenames/hold_a1r9v-0pq.bak",
		"chaotic_filenames/keep_4xv7__m2.log",
		"chaotic_filenames/omega_77xy__aa.bin",
		"chaotic_filenames/xqwe_90210.md",
		"chaotic_filenames/zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));

	ASSERT(SUCCESS == delete_path(db_filename));

	RETURN_STATUS;
}

/**
 * @brief Validate update included branch in three passes
 */
static Return test0009_4(void)
{
	INITTEST;

	const char *db_filename = "database0009_4.db";

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/ignore_include_cases/chaotic_filenames"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments_create = "--database=database0009_4.db "
	        "--ignore=\"^(?:skip_|tmp_).+\" "
	        "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	/*
	 * Create the baseline DB using the same ignore/include rules as the later update passes
	 * We intentionally build the initial record set as "tracked-after-filters" and not as "all files in directory"
	 * Update mode processes the current filtered set and does not retroactively delete rows that were inserted earlier
	 * If this first pass omitted filters, all 12 paths would be stored and the update-included scenario would validate a different logic branch
	 */
	const struct argument_test_case create_cases[] = {
		{arguments_create,COMPLETED,"templates/0009_004_1.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(create_cases,NULL,NULL));

	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/skip_4xv7__m2.log"));

	const char *arguments_update = "--update --check-level=QUICK --database=database0009_4.db "
	        "--ignore=\"^(?:skip_|tmp_).+\" "
	        "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	// Update mode pass where only one included file has changed
	const struct argument_test_case update_cases[] = {
		{arguments_update,COMPLETED,"templates/0009_004_2.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(update_cases,NULL,NULL));

	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/tmp_qwe_90210.log"));
	ASSERT(SUCCESS == add_string_to(" ","tests/fixtures/ignore_include_cases/chaotic_filenames/tmp_z1-9vv.bak"));
	// Truncate a tracked non-included file to trigger the "updated as empty" branch
	ASSERT(SUCCESS == truncate_file_to_zero_size("tests/fixtures/ignore_include_cases/chaotic_filenames/alpha_m0n9k2_zz.txt"));

	const char *arguments_update_watch = "--watch-timestamps --update --check-level=QUICK --database=database0009_4.db "
	        "--ignore=\"^(?:skip_|tmp_).+\" "
	        "--include=\"^(?:skip_4xv7__m2\\.log|tmp_qwe_90210\\.log|tmp_z1-9vv\\.bak)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	// Update mode with watch-timestamps enabled where one non-included file becomes empty and two included files are updated
	const struct argument_test_case watch_timestamps_cases[] = {
		{arguments_update_watch,COMPLETED,"templates/0009_004_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(watch_timestamps_cases,NULL,NULL));

	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"omega_77xy__aa.bin",
		"skip_4xv7__m2.log",
		"tmp_qwe_90210.log",
		"tmp_z1-9vv.bak",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == delete_path(db_filename));

	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/ignore_include_cases/chaotic_filenames"));

	RETURN_STATUS;
}

/**
 * @brief Verify ignored missing DB rows stay outside cleanup without --db-drop-ignored
 *
 * The test creates a database with all fixture files, removes one file below a
 * later ignored subtree, then runs update with the matching --ignore pattern.
 * The deleted ignored row must remain in the DB and must not be reported as
 * missing-file cleanup
 */
static Return test0009_5(void)
{
	INITTEST;

	const char *db_file_name = "database0009_5.db";
	const char *ignored_relative_path = "path2/AAA/ZAW/D/e/f/b_file.txt";
	const char *arguments = NULL;
	bool ignored_row_exists = false;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database0009_5.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	arguments = "--update --check-level=QUICK --ignore=\"^path2/AAA/ZAW/.*\" "
	        "--database=database0009_5.db tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_005.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == db_relative_path_exists(db_file_name,ignored_relative_path,&ignored_row_exists));
	ASSERT(ignored_row_exists == true);

	ASSERT(SUCCESS == delete_path(db_file_name));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify --include restores an ignored missing DB row to cleanup scope
 *
 * The test deletes a file that matches both --ignore and --include during the
 * update run. The include pattern must make cleanup treat the missing row as a
 * normal tracked path, delete it from the DB, and report it as no longer present
 */
static Return test0009_6(void)
{
	INITTEST;

	const char *db_file_name = "database0009_6.db";
	const char *included_relative_path = "path2/AAA/ZAW/D/e/f/b_file.txt";
	const char *arguments = NULL;
	bool included_row_exists = true;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database0009_6.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	arguments = "--update --check-level=QUICK --ignore=\"^path2/AAA/ZAW/.*\" "
	        "--include=\"^path2/AAA/ZAW/D/e/f/b_file\\.txt$\" "
	        "--database=database0009_6.db tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_006.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == db_relative_path_exists(db_file_name,included_relative_path,&included_row_exists));
	ASSERT(included_row_exists == false);

	ASSERT(SUCCESS == delete_path(db_file_name));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify --db-drop-ignored removes an ignored missing DB row as ignored
 *
 * A deleted file row that matches --ignore must be classified as ignored cleanup
 * when --db-drop-ignored is present, not as ordinary missing-file cleanup.
 * CLI and configuration updates use the same saved database and deleted file,
 * then check the full output and removal of the ignored record
 */
static Return test0009_7(void)
{
	INITTEST;

	const char *db_file_name = "database0009_7.db";
	const char *db_backup_file_name = "database0009_7_before_update.db";
	const char *ignored_relative_path = "path2/AAA/ZAW/D/e/f/b_file.txt";
	const char *arguments = NULL;
	bool ignored_row_exists = true;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database0009_7.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	/* Preserve the database before either update removes the ignored record */
	ASSERT(SUCCESS == copy_path(db_file_name,db_backup_file_name));

	arguments = "--update --check-level=QUICK --db-drop-ignored "
	        "--ignore=\"^path2/AAA/ZAW/D/e/f/b_file\\.txt$\" "
	        "--database=database0009_7.db tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_007.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == db_relative_path_exists(db_file_name,ignored_relative_path,&ignored_row_exists));
	ASSERT(ignored_row_exists == false);

	/* Restore only the database; the file remains deleted for the configuration run */
	ASSERT(SUCCESS == copy_path(db_backup_file_name,db_file_name));
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0009_007.conf",COMPLETED,"templates/0009_007.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	ASSERT(SUCCESS == db_relative_path_exists(db_file_name,ignored_relative_path,&ignored_row_exists));
	ASSERT(ignored_row_exists == false);
	call(delete_path_if_present(db_backup_file_name));

	ASSERT(SUCCESS == delete_path(db_file_name));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify --db-drop-ignored removes an ignored existing DB row
 *
 * The ignored file still exists on disk, so this covers explicit ignored
 * cleanup independently from missing-file cleanup
 */
static Return test0009_8(void)
{
	INITTEST;

	const char *db_file_name = "database0009_8.db";
	const char *ignored_relative_path = "path2/AAA/ZAW/D/e/f/b_file.txt";
	const char *arguments = NULL;
	bool ignored_row_exists = true;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database0009_8.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --check-level=QUICK --db-drop-ignored "
	        "--ignore=\"^path2/AAA/ZAW/D/e/f/b_file\\.txt$\" "
	        "--database=database0009_8.db tests/fixtures/diffs/diff1";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_008.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == db_relative_path_exists(db_file_name,ignored_relative_path,&ignored_row_exists));
	ASSERT(ignored_row_exists == false);

	ASSERT(SUCCESS == delete_path(db_file_name));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Validate quiet ignore reporting through CLI and configuration
 *
 * Both scans start without a database and use the same full output template.
 * Ignored-file lines must be absent while the remaining messages and exact
 * stored paths match the ordinary ignore-filtered scan
 *
 * @return Return status for equivalent quiet reporting and stored paths
 */
static Return test0009_9(void)
{
	INITTEST;
	const char *db_filename = "database0009_9.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--database=database0009_9.db --quiet-ignored "
	        "--ignore=\"^(?:skip_|tmp_).*\\.(?:log|bak)$\" "
	        "tests/fixtures/ignore_include_cases/chaotic_filenames";

	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,"templates/0009_009.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	const char *expected_paths[] =
	{
		"alpha_m0n9k2_zz.txt",
		"hold_a1r9v-0pq.bak",
		"keep_4xv7__m2.log",
		"omega_77xy__aa.bin",
		"xqwe_90210.md",
		"zeta_z1-9vv.dat"
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == delete_path(db_filename));

	/* Repeat the scan with the same quiet setting and ignore rule from configuration */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0009_009.conf",COMPLETED,"templates/0009_009.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,(int)(sizeof(expected_paths) / sizeof(expected_paths[0]))));
	ASSERT(SUCCESS == delete_path(db_filename));

	RETURN_STATUS;
}

Return test0009(void)
{
	INITTEST;

	TEST(test0009_1,"Ignore regexp splits chaotic filenames into tracked and skipped sets");
	TEST(test0009_2,"Ignore most files and include back selected ones");
	TEST(test0009_3,"Directory ignore with selective child include");
	TEST(test0009_4,"Create then update included files with and without detailed change output");
	TEST(test0009_5,"Ignored missing DB rows stay outside cleanup without --db-drop-ignored");
	TEST(test0009_6,"--include restores an ignored missing DB row to cleanup scope");
	TEST(test0009_7,"--db-drop-ignored removes an ignored missing DB row as ignored");
	TEST(test0009_8,"--db-drop-ignored removes an ignored existing DB row");
	TEST(test0009_9,"--quiet-ignored hides ignored-file lines without changing stored paths through CLI and configuration");

	RETURN_STATUS;
}
