#include "sute.h"

/**
 * The db file should not be created in the Dry Run mode
 */
static Return test0013_1(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	m_del(result);

	// Does file exists or not
	const char *db_filename = "database1.db";
	m_create(char,path,MEMORY_STRING);
	bool file_exists = false;

	ASSERT(SUCCESS == construct_path(db_filename,path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(path)));

	m_del(path);

	// Should not be exists
	ASSERT(file_exists == false);

	RETURN_STATUS;
}

/**
 * In dry-run mode with checksums, files are hashed but DB is still not modified
 */
static Return test0013_2(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,path,MEMORY_STRING);
	bool file_exists = false;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run --database=dry_run_regular.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0013_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	arguments = "--dry-run=with-checksums --database=dry_run_with_checksums.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0013_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);
	m_del(path);

	/* Dry-run should not create either DB file. */
	ASSERT(SUCCESS == construct_path("dry_run_regular.db",path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(path)));
	ASSERT(file_exists == false);

	m_del(path);
	ASSERT(SUCCESS == construct_path("dry_run_with_checksums.db",path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,m_text(path)));
	ASSERT(file_exists == false);

	m_del(path);

	RETURN_STATUS;
}

/**
 * Invalid dry-run mode should fail with error in stderr
 */
static Return test0013_3(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run=bad-mode --database=dry_run_invalid.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,result,FAILURE,STDERR_ALLOW));

	const char *filename = "templates/0013_001_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * The db file should not be updated in the Dry Run mode
 */
static Return test0013_4(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	struct stat stat1;
	struct stat stat2;
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,chunk,MEMORY_STRING);

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	printf("Path: %s\n",path);
	echo(STDOUT,"Path: %s\n",path);
	#endif

	m_create(char,path,MEMORY_STRING);

	const char *db_filename = "database1.db";

	ASSERT(SUCCESS == construct_path(db_filename,path));

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat1));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt")); // Remove
	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt")); // Modify
	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt")); // New file

	arguments = "--dry-run --update --database=database1.db"
	        " tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_copy(result,chunk));

	#if 0
	printf("%s\n",m_text(result));
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	m_del(result);
	m_del(chunk);

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Compare against the sample. A message should be displayed indicating
	// that the --db-drop-ignored option must be specified for permanent
	// removal of ignored files from the database
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --ignore=\"^1/AAA/ZAW/.*\" --update "
	        "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	const char *filename = "templates/0013_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Dry Run mode permanent deletion of all ignored file
	// references from the database
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --db-drop-ignored --update"
	        " --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\..*\""
	        " --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	filename = "templates/0013_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --db-drop-ignored --update --watch-timestamps"
	        " --ignore=\"^path2/AAA/ZAW/.*\""
	        " --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	filename = "templates/0013_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	m_del(path);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * Everything that was previously executed in Dry Run mode
 * will now be tested in live mode without simulation, and
 * the results will be compared against each other
 */
static Return test0013_5(void)
{
	INITTEST;
	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,path,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	const char *db_file_name = "database1.db";
	const char *arguments = NULL;

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == construct_path(db_file_name,path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"Path: %s\n",path);
	#endif

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt"));
	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));
	ASSERT(SUCCESS == replase_to_string("WNEURHGO","tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/b.txt")); // New file

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	m_del(result);

	// Compare against the sample. A message should be displayed indicating
	// that the --db-drop-ignored option must be specified for permanent
	// removal of ignored files from the database
	ASSERT(SUCCESS == copy_path("database1.db","database1.db.backup"));

	arguments = "--ignore=\"^1/AAA/ZAW/.*\" --update --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	const char *filename = "templates/0013_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	// Real live mode permanent deletion of all ignored file
	// references from the database
	ASSERT(SUCCESS == copy_path("database1.db.backup","database1.db"));

	arguments = "--db-drop-ignored --update"
	        " --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\..*\""
	        " --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	filename = "templates/0013_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	ASSERT(SUCCESS == move_path("database1.db.backup","database1.db"));

	arguments = "--watch-timestamps --db-drop-ignored "
	        "--ignore=\"^path2/AAA/ZAW/.*\" --update "
	        "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",m_text(result));
	#endif

	filename = "templates/0013_003_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);

	m_del(result);

	m_del(path);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 * @brief Verify diff generation for dry-run and writable database scenarios
 *
 * This test loads three pairs of saved outputs into string-mode memory
 * descriptors and compares each pair through compare_memory_strings().
 * The produced unified diff is matched against the corresponding template
 *
 * The test verifies that compare_memory_strings():
 * - accepts file content loaded into string-mode memory descriptors
 * - writes the produced unified diff directly into the destination descriptor
 * - produces the exact diff text expected for the compared application runs
 *
 * @return Return status code
 */
Return test0013_6(void)
{
	INITTEST;

	m_create(char,text1,MEMORY_STRING);
	m_create(char,text2,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,diff_buffer,MEMORY_STRING);
	const char *filename = NULL;

	/* 0013 002 1 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_1.txt",text2));

	ASSERT(SUCCESS == compare_memory_strings(diff_buffer,text1,text2));

	filename = "templates/0013_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	m_del(text1);
	m_del(text2);
	m_del(pattern);

	/* 0013 002 2 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_2.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_2.txt",text2));

	m_del(diff_buffer);
	ASSERT(SUCCESS == compare_memory_strings(diff_buffer,text1,text2));

	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	m_del(text1);
	m_del(text2);
	m_del(pattern);

	/* 0013 002 3 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_3.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_3.txt",text2));

	m_del(diff_buffer);
	ASSERT(SUCCESS == compare_memory_strings(diff_buffer,text1,text2));

	// _2 is not a mistake. 2 and 3 are equals
	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	m_del(text1);
	m_del(text2);
	m_del(pattern);
	m_del(diff_buffer);

	RETURN_STATUS;
}

/**
 * Verifies detection of unexpected database metadata drift in dry-run mode.
 *
 * The function performs two runs. First, it creates `database1.db` in normal
 * mode. Then it enables the testing hook
 * `TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED=true` and runs the application
 * with `--dry-run --update`. In test-hook mode this forces a DB timestamp bump
 * inside the process before `db_check_changes()` compares the saved and current
 * file metadata.
 *
 * The expected outcome of the second run is `WARNING`
 */
static Return test0013_7(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	const char *arguments = NULL;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));

	/* First run: create DB in normal mode. */
	arguments = "--database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED","true"));
	arguments = "--dry-run --update --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	const char *filename = "templates/0013_005.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));
	ASSERT(SUCCESS == delete_path("database1.db"));

	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * Verifies internal consistency check when DB should be modified but appears unchanged.
 *
 * The function first validates a real update path: it creates `database1.db`,
 * modifies one sample file, runs `--update`, compares output with template,
 * and verifies that DB file metadata has actually changed.
 *
 * After that, it restores the pre-update DB state, enables
 * `TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED=true`, and runs `--update`
 * again. The hook rewrites the saved baseline database stat to the current
 * stat right before comparison in db_check_changes(). This simulates a faulty
 * "no metadata drift" result after a real database update and must trigger
 * WARNING.
 */
static Return test0013_8(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,path,MEMORY_STRING);

	struct stat stat_before_real_update = {0};
	struct stat stat_after_real_update = {0};

	const char *arguments = NULL;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED","false"));

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/* First run: create DB in normal mode. */
	arguments = "--database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Make at least one real filesystem change so --update modifies DB. */
	ASSERT(SUCCESS == add_string_to("AFAKDSJ","tests/fixtures/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt"));

	/* Control check: real --update must modify database metadata. */
	ASSERT(SUCCESS == construct_path("database1.db",path));
	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat_before_real_update));

	ASSERT(SUCCESS == copy_path("database1.db","database1.db.backup"));

	arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0013_006_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == get_file_stat(m_text(path),&stat_after_real_update));
	ASSERT(FAILURE == check_file_identity(&stat_before_real_update,&stat_after_real_update));

	/*
	 * Restore pre-update DB snapshot so the next run starts from the same state
	 * and tests only the simulation hook behavior.
	 */
	ASSERT(SUCCESS == move_path("database1.db.backup","database1.db"));

	m_del(result);
	m_del(pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED","true"));
	arguments = "--update --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0013_006_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_DB_FILE_STAT_WILL_BE_RESYNCED","false"));
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	m_del(path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 *
 * Dry Run mode testing
 *
 */
Return test0013(void)
{
	INITTEST;

	TEST(test0013_1,"The DB file should not be created");
	TEST(test0013_2,"Dry run with checksums hashes files but keeps DB untouched");
	TEST(test0013_3,"Invalid dry-run mode should return failure and print stderr error");
	TEST(test0013_4,"The DB file should not be updated");
	TEST(test0013_5,"Now run the same without simulation");
	TEST(test0013_6,"Compare dry and real mode templates");
	TEST(test0013_7,"Dry-run DB metadata drift should trigger internal warning path");
	TEST(test0013_8,"Live update: force missing DB metadata drift and trigger warning");

	RETURN_STATUS;
}
