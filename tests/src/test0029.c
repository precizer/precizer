#include "sute.h"

/**
 * @brief Verify reporting and preservation of inaccessible paths
 *
 * Makes one file and one directory inaccessible, runs an update without
 * `--db-drop-inaccessible`, and verifies that the expected inaccessible-path
 * messages are produced while their database records are retained
 */
static Return test0029_1(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	const char *db_backup_filename = "0029_001_before_output.db";

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* Save the database before any output variant processes the inaccessible paths */
	ASSERT(SUCCESS == copy_path("database1.db",db_backup_filename));

	arguments = "--update --check-level=QUICK --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/* Both records must survive with the metadata and checksum saved before chmod */
	const char *inaccessible_paths[] = {"1/AAA/ZAW/D/e/f/b_file.txt","2/AAA/BBB/CZC/a.txt"};
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--update --check-level=QUICK --database=database1.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_001_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--update --check-level=QUICK --database=database1.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_001_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == copy_path(db_backup_filename,"database1.db"));
		const struct argument_test_case single_case[] = {output_cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		int files_count = 0;
		ASSERT(SUCCESS == db_read_files_count("database1.db",&files_count));
		ASSERT(files_count == 12);

		/* Output formatting must not rewrite either inaccessible record */
		for(size_t j = 0; j < sizeof(inaccessible_paths) / sizeof(inaccessible_paths[0]) && SUCCESS == status; ++j)
		{
			CmpctStat before = {0},after = {0};
			sqlite3_int64 offset_before = 0,offset_after = 0;
			unsigned char hash_before[SHA512_DIGEST_LENGTH],hash_after[SHA512_DIGEST_LENGTH];
			ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_backup_filename,inaccessible_paths[j],&before));
			ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path("database1.db",inaccessible_paths[j],&after));
			ASSERT(memcmp(&before,&after,sizeof(before)) == 0);
			ASSERT(SUCCESS == read_final_sha512_from_db(db_backup_filename,inaccessible_paths[j],&offset_before,hash_before));
			ASSERT(SUCCESS == read_final_sha512_from_db("database1.db",inaccessible_paths[j],&offset_after,hash_after));
			ASSERT(offset_before == offset_after);
			ASSERT(memcmp(hash_before,hash_after,sizeof(hash_before)) == 0);
		}
	}
	call(delete_path_if_present(db_backup_filename));

	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0666));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0777));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify removal of inaccessible records with --db-drop-inaccessible
 *
 * Makes one file and one directory inaccessible, enables
 * `--db-drop-inaccessible`, and verifies that their database records are
 * removed during the update. CLI and configuration runs use the same saved
 * database and access restrictions, then check the same full output
 */
static Return test0029_2(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	const char *db_backup_filename = "0029_002.db";

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	/* Preserve the prepared database before either update removes inaccessible records */
	ASSERT(SUCCESS == copy_path("database2.db",db_backup_filename));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --check-level=QUICK --db-drop-inaccessible --database=database2.db "
	        "tests/fixtures/diffs/diff1";

	const char *filename = "templates/0029_002.txt";
	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	/* Restore only the database; access restrictions remain for the configuration run */
	ASSERT(SUCCESS == copy_path(db_backup_filename,"database2.db"));
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0029_002.conf",COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	/* Repeat the removal from the same saved rows and unchanged access restrictions */
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--update --check-level=QUICK --db-drop-inaccessible --database=database2.db "
		 "tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_002_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--update --check-level=QUICK --db-drop-inaccessible --database=database2.db "
		 "tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_002_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == copy_path(db_backup_filename,"database2.db"));
		const struct argument_test_case single_case[] = {output_cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		bool row_exists = true;
		ASSERT(SUCCESS == db_relative_path_exists("database2.db","1/AAA/ZAW/D/e/f/b_file.txt",&row_exists));
		ASSERT(row_exists == false);
		ASSERT(SUCCESS == db_relative_path_exists("database2.db","2/AAA/BBB/CZC/a.txt",&row_exists));
		ASSERT(row_exists == false);
		int files_count = 0;
		ASSERT(SUCCESS == db_read_files_count("database2.db",&files_count));
		ASSERT(files_count == 10);
	}
	call(delete_path_if_present(db_backup_filename));

	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0666));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0777));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify that an access-check failure for one regular file does not stop traversal
 *
 * Forces one early file to report FILE_ACCESS_ERROR during an update run, then
 * changes a later file. The update for the later file proves that traversal
 * continued after the access-check failure
 */
static Return test0029_3(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *db_filename = "database3.db";
	const char *db_backup_filename = "0029_003_before_output.db";
	m_create(char,changed_path,MEMORY_STRING);
	const char *access_error_root_path = "tests/fixtures/diffs/diff1";
	const char *access_error_relative_path = "1/AAA/ZAW/D/e/f/b_file.txt";
	const char *changed_later_file_path = "tests/fixtures/diffs/diff1/4/AAA/BBB/CCC/a.txt";

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database3.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == replase_to_string("access check failure must not stop traversal",changed_later_file_path));

	ASSERT(SUCCESS == expect_file_access_from_root(
		access_error_root_path,
		access_error_relative_path,
		R_OK,
		FILE_ACCESS_ALLOWED));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",access_error_relative_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(SUCCESS == expect_file_access_from_root(
		access_error_root_path,
		access_error_relative_path,
		R_OK,
		FILE_ACCESS_ERROR));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* Preserve the pre-update row so every format observes the same content change */
	ASSERT(SUCCESS == copy_path(db_filename,db_backup_filename));

	arguments = "--update --check-level=QUICK --database=database3.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	const char *filename = "templates/0029_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/* Re-enable the same access failure for every output format */
	ASSERT(SUCCESS == construct_path(changed_later_file_path,changed_path));
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--update --check-level=QUICK --database=database3.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_003_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--update --check-level=QUICK --database=database3.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_003_3.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine --verbose " "--update --check-level=QUICK --database=database3.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_003_4.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
		ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",access_error_relative_path));
		ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
		const struct argument_test_case single_case[] = {output_cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		call(set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
		call(set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

		/* The failed access check must retain its row and allow the later update */
		bool row_exists = false;
		ASSERT(SUCCESS == db_relative_path_exists(db_filename,access_error_relative_path,&row_exists));
		ASSERT(row_exists == true);
		ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,"4/AAA/BBB/CCC/a.txt",m_text(changed_path)));
		ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,"4/AAA/BBB/CCC/a.txt"));
	}
	call(delete_path_if_present(db_backup_filename));
	m_del(changed_path);

	m_del(pattern);
	m_del(result);

	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify that every root opening failure safely skips metadata cleanup
 *
 * Creates a baseline database, skips file traversal, and forces root opening
 * to report access denied, not found, and an unexpected access error. Every
 * update uses `--db-drop-inaccessible` and must still finish successfully
 * without deleting file records. The complete output from every update is
 * matched against the same PCRE2 template
 */
static Return test0029_4(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *db_filename = "database4.db";
	const char *db_backup_filename = "0029_004_before_output.db";
	const char *filename = "templates/0029_004.txt";
	const char *root_access_statuses[] = {
		"FILE_ACCESS_DENIED",
		"FILE_NOT_FOUND",
		"FILE_ACCESS_ERROR"
	};
	const size_t root_access_status_count =
	        sizeof(root_access_statuses) / sizeof(root_access_statuses[0]);
	int files_count_before = 0;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database4.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(result->length == 0);
	ASSERT(SUCCESS == db_read_files_count(db_filename,&files_count_before));
	ASSERT(files_count_before > 0);

	/* Each root-access failure starts with all original rows */
	ASSERT(SUCCESS == copy_path(db_filename,db_backup_filename));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST","1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX","diff1"));
	ASSERT(SUCCESS == get_file_content(filename,pattern));

	arguments = "--update --check-level=QUICK --db-drop-inaccessible --database=database4.db "
	        "tests/fixtures/diffs/diff1";

	/* Only the initialization diagnostic precedes the concise logger selection */
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--update --check-level=QUICK --db-drop-inaccessible --database=database4.db "
		 "tests/fixtures/diffs/diff1",COMPLETED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--update --check-level=QUICK --db-drop-inaccessible --database=database4.db "
		 "tests/fixtures/diffs/diff1",COMPLETED,"templates/0045_033.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t index = 0; index < root_access_status_count; index++)
	{
		int files_count_after = 0;

		ASSERT(SUCCESS == set_environment_variable(
			"TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",
			root_access_statuses[index]));

		ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
		ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
		ASSERT(SUCCESS == match_pattern(result,pattern,filename));

		ASSERT(SUCCESS == db_read_files_count(db_filename,&files_count_after));
		ASSERT(files_count_after == files_count_before);

		/* Root failures suppress cleanup in compact and machine as well */
		for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
		{
			ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
			const struct argument_test_case single_case[] = {output_cases[i],{0}};
			ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
			ASSERT(SUCCESS == db_read_files_count(db_filename,&files_count_after));
			ASSERT(files_count_after == files_count_before);
		}

	}

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	call(delete_path_if_present(db_backup_filename));
	m_del(pattern);
	m_del(result);

	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify that an unavailable PATH root does not stop later roots
 *
 * Forces the first of two traversal roots to fail during root opening. The
 * unavailable root must be reported immediately and shown again in the final
 * warning summary, while a file from the second root must still be written to
 * the database. A configuration-file run repeats both checks with the same
 * access failure after removing the database created by the CLI run
 */
static Return test0029_5(void)
{
	INITTEST;

	const char *db_filename = "database5.db";
	const char *filename = "templates/0029_005.txt";
	const char *available_relative_path = "AAA/BBB/CCC/a.txt";
	bool available_path_exists = false;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX","diff1/1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_DENIED"));

	const char *arguments = "--database=database5.db "
	        "tests/fixtures/diffs/diff1/1 tests/fixtures/diffs/diff1/4";
	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};

	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,available_relative_path,&available_path_exists));
	ASSERT(available_path_exists == true);

	/* Preserve the unavailable-root condition for the configuration-file repeat */
	ASSERT(SUCCESS == delete_path(db_filename));
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0029_005.conf",COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	available_path_exists = false;
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,available_relative_path,&available_path_exists));
	ASSERT(available_path_exists == true);

	/* Start each concise scan without a DB and retain the unavailable first root */
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--database=database5.db "
		 "tests/fixtures/diffs/diff1/1 tests/fixtures/diffs/diff1/4",COMPLETED,"templates/0029_005_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--database=database5.db "
		 "tests/fixtures/diffs/diff1/1 tests/fixtures/diffs/diff1/4",COMPLETED,"templates/0029_005_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == delete_path_if_present(db_filename));
		const struct argument_test_case single_case[] = {output_cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		const char *expected_paths[] = {available_relative_path};
		ASSERT(SUCCESS == db_paths_match(db_filename,expected_paths,1));
	}

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	ASSERT(SUCCESS == delete_path(db_filename));

	RETURN_STATUS;
}

/**
 * @brief Verify --db-drop-inaccessible does not override ignore scope
 *
 * Makes one file inaccessible and ignores the same relative path during update.
 * Even with --db-drop-inaccessible, the ignored non-locked row must stay in the
 * DB and must not be reported through inaccessible cleanup. CLI and configuration
 * runs check the same full output and preserve the same database record
 */
static Return test0029_6(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);

	const char *db_filename = "database6.db";
	const char *db_backup_filename = "0029_006_before_output.db";
	const char *ignored_relative_path = "1/AAA/ZAW/D/e/f/b_file.txt";
	bool ignored_path_exists = false;

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database6.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/* Keep the same ignored row and permission change for every format */
	ASSERT(SUCCESS == copy_path(db_filename,db_backup_filename));

	arguments = "--update --check-level=QUICK --db-drop-inaccessible "
	        "--ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" "
	        "--database=database6.db tests/fixtures/diffs/diff1";

	const char *filename = "templates/0029_006.txt";
	const struct argument_test_case cases[] = {
		{arguments,COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));

	ASSERT(SUCCESS == db_relative_path_exists(db_filename,ignored_relative_path,&ignored_path_exists));
	ASSERT(ignored_path_exists == true);

	/* The CLI update leaves the database unchanged, so reuse it with the same permissions */
	const struct argument_test_case file_cases[] = {
		{"--config=tests/templates/0029_006.conf",COMPLETED,filename,STDERR_MUST_BE_EMPTY},
		{0}
	};
	ASSERT(SUCCESS == run_and_match_templates(file_cases,NULL,NULL));
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,ignored_relative_path,&ignored_path_exists));
	ASSERT(ignored_path_exists == true);

	/* The ignored inaccessible file must remain outside explicit deletion scope */
	const struct argument_test_case output_cases[] = {
		{"--output=compact " "--update --check-level=QUICK --db-drop-inaccessible "
		 "--ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" "
		 "--database=database6.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_006_2.txt",STDERR_MUST_BE_EMPTY},
		{"--output=machine " "--update --check-level=QUICK --db-drop-inaccessible "
		 "--ignore=\"^1/AAA/ZAW/D/e/f/b_file\\.txt$\" "
		 "--database=database6.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0029_006_3.txt",STDERR_MUST_BE_EMPTY},
		{0}
	};

	for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
	{
		ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
		const struct argument_test_case single_case[] = {output_cases[i],{0}};
		ASSERT(SUCCESS == run_and_match_templates(single_case,NULL,NULL));
		ASSERT(SUCCESS == db_relative_path_exists(db_filename,ignored_relative_path,&ignored_path_exists));
		ASSERT(ignored_path_exists == true);
	}
	call(delete_path_if_present(db_backup_filename));

	m_del(result);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0666));
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Run inaccessible-path behavior tests
 *
 * Covers preservation, explicit removal, unexpected file access-check failure,
 * safe cleanup skipping, and continued traversal after a PATH root cannot be
 * opened
 */
Return test0029(void)
{
	INITTEST;

	TEST(test0029_1,"Inaccessible paths are reported and kept by default");
	TEST(test0029_2,"--db-drop-inaccessible drops inaccessible DB records");
	TEST(test0029_3,"Access-check failure for a regular file does not stop traversal");
	TEST(test0029_4,"Root opening failures skip metadata cleanup without deleting records");
	TEST(test0029_5,"Unavailable PATH root is remembered while later roots continue");
	TEST(test0029_6,"--db-drop-inaccessible does not override ignore scope");

	RETURN_STATUS;
}
