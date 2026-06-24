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

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

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
 * removed during the update
 */
static Return test0029_2(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --database=database2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(result->length == 0);

	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt",0000));
	ASSERT(SUCCESS == change_mode("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC",0000));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--update --db-drop-inaccessible --database=database2.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0029_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
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

	arguments = "--update --database=database3.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	const char *filename = "templates/0029_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

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

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST","1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX","diff1"));
	ASSERT(SUCCESS == get_file_content(filename,pattern));

	arguments = "--update --db-drop-inaccessible --database=database4.db "
	        "tests/fixtures/diffs/diff1";

	for(size_t index = 0; index < root_access_status_count; index++)
	{
		int files_count_after = 0;

		ASSERT(SUCCESS == set_environment_variable(
			"TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",
			root_access_statuses[index]));

		ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
		ASSERT(SUCCESS == match_pattern(result,pattern,filename));

		ASSERT(SUCCESS == db_read_files_count(db_filename,&files_count_after));
		ASSERT(files_count_after == files_count_before);
	}

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

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
 * the database
 */
static Return test0029_5(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *db_filename = "database5.db";
	const char *filename = "templates/0029_005.txt";
	const char *available_relative_path = "AAA/BBB/CCC/a.txt";
	bool available_path_exists = false;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX","diff1/1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_DENIED"));

	const char *arguments = "--database=database5.db "
	        "tests/fixtures/diffs/diff1/1 tests/fixtures/diffs/diff1/4";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,available_relative_path,&available_path_exists));
	ASSERT(available_path_exists == true);

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	m_del(pattern);
	m_del(result);

	ASSERT(SUCCESS == delete_path(db_filename));

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

	RETURN_STATUS;
}
