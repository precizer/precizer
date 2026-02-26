#include "sute.h"

/**
 * The db file should not be created in the Dry Run mode
 */
static Return test0013_1(void)
{
	INITTEST;

	create(char,result);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	del(result);

	// Does file exists or not
	const char *db_filename = "database1.db";
	create(char,path);
	bool file_exists = false;

	ASSERT(SUCCESS == construct_path(db_filename,path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(path)));

	del(path);

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

	create(char,result);
	create(char,pattern);
	create(char,path);
	bool file_exists = false;

	const char *command = "cd ${TMPDIR}; rm -f dry_run_regular.db dry_run_with_checksums.db;";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run --database=dry_run_regular.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0013_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	arguments = "--dry-run=with-checksums --database=dry_run_with_checksums.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0013_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);
	del(path);

	/* Dry-run should not create either DB file. */
	ASSERT(SUCCESS == construct_path("dry_run_regular.db",path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(path)));
	ASSERT(file_exists == false);

	del(path);
	ASSERT(SUCCESS == construct_path("dry_run_with_checksums.db",path));
	ASSERT(SUCCESS == check_file_exists(&file_exists,getcstring(path)));
	ASSERT(file_exists == false);

	del(path);

	RETURN_STATUS;
}

/**
 * Invalid dry-run mode should fail with error in stderr
 */
static Return test0013_3(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--dry-run=bad-mode --database=dry_run_invalid.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,result,FAILURE,STDERR_ALLOW));

	const char *filename = "templates/0013_001_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * The db file should not be updated in the Dry Run mode
 */
static Return test0013_4(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	struct stat stat1;
	struct stat stat2;
	create(char,pattern);
	create(char,chunk);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup;"
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	printf("Path: %s\n",path);
	echo(STDOUT,"Path: %s\n",path);
	#endif

	create(char,path);

	const char *db_filename = "database1.db";

	ASSERT(SUCCESS == construct_path(db_filename,path));

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat1));

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" // Remove
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Modify
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--dry-run --update --database=database1.db"
	        " tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	#if 0
	printf("%s\n",getcstring(result));
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	del(result);
	del(chunk);

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Compare against the sample. A message should be displayed indicating
	// that the --db-drop-ignored option must be specified for permanent
	// removal of ignored files from the database
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --ignore=\"^1/AAA/ZAW/.*\" --update "
	        "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	const char *filename = "templates/0013_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Dry Run mode permanent deletion of all ignored file
	// references from the database
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --db-drop-ignored --update"
	        " --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\..*\""
	        " --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	filename = "templates/0013_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--dry-run --db-drop-ignored --update --watch-timestamps"
	        " --ignore=\"^path2/AAA/ZAW/.*\""
	        " --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	filename = "templates/0013_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	del(path);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db && "
	        "rm -rf tests/examples/diffs/diff1 && "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

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
	create(char,result);
	create(char,path);
	create(char,pattern);
	const char *db_file_name = "database1.db";
	const char *command = NULL;
	const char *arguments = NULL;

	// Preparation for tests
	command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup;"
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == construct_path(db_file_name,path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	arguments = "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"Path: %s\n",path);
	#endif

	command = "cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;"
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;"
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	del(result);

	// Compare against the sample. A message should be displayed indicating
	// that the --db-drop-ignored option must be specified for permanent
	// removal of ignored files from the database
	command = "cd ${TMPDIR};"
	        "cp -a database1.db database1.db.backup;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--ignore=\"^1/AAA/ZAW/.*\" --update --database=database1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	const char *filename = "templates/0013_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	// Real live mode permanent deletion of all ignored file
	// references from the database
	command = "cd ${TMPDIR};"
	        "cp -a database1.db.backup database1.db;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--db-drop-ignored --update"
	        " --ignore=\"^1/AAA/ZAW/D/e/f/b_file\\..*\""
	        " --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	filename = "templates/0013_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	command = "cd ${TMPDIR};"
	        "mv database1.db.backup database1.db;";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--watch-timestamps --db-drop-ignored "
	        "--ignore=\"^path2/AAA/ZAW/.*\" --update "
	        "--database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	#if 0
	echo(STDOUT,"%s\n",getcstring(result));
	#endif

	filename = "templates/0013_003_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);

	del(result);

	del(path);

	// Clean up test results
	command = "cd ${TMPDIR} && "
	        "rm database1.db && "
	        "rm -rf tests/examples/diffs/diff1 && "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

Return test0013_6(void)
{
	INITTEST;

	create(char,text1);
	create(char,text2);
	char *diff = NULL;
	create(char,pattern);
	const char *filename = NULL;

	/* 0013 002 1 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_1.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_1.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	filename = "templates/0013_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	create(char,diff_buffer);
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	reset(&diff);
	del(pattern);

	/* 0013 002 2 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_2.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_2.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	del(diff_buffer);
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	reset(&diff);
	del(pattern);

	/* 0013 002 3 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_3.txt",text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_3.txt",text2));

	ASSERT(SUCCESS == compare_strings(&diff,getcstring(text1),getcstring(text2)));

	// _2 is not a mistake. 2 and 3 are equals
	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	del(diff_buffer);
	ASSERT(SUCCESS == copy_literal(diff_buffer,diff));
	ASSERT(SUCCESS == match_pattern(diff_buffer,pattern,filename));

	del(text1);
	del(text2);
	reset(&diff);
	del(pattern);
	del(diff_buffer);

	RETURN_STATUS;
}

/**
 * Verifies detection of unexpected database metadata drift in dry-run mode.
 *
 * The function performs two runs. First, it creates `database1.db` in normal
 * mode. Then it enables the testing hook
 * `PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED=true` and runs the application
 * with `--dry-run --update`. In test-hook mode this forces a DB timestamp bump
 * inside the process before `db_check_changes()` compares the saved and current
 * file metadata.
 *
 * The expected outcome of the second run is `WARNING`
 */
static Return test0013_7(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	const char *cleanup_command = "cd ${TMPDIR}; rm -f database1.db;";
	const char *arguments = NULL;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));

	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* First run: create DB in normal mode. */
	arguments = "--database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED","true"));
	arguments = "--dry-run --update --database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	const char *filename = "templates/0013_005.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));
	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

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
 * `PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED=true`, and runs `--update`
 * again. The hook rewrites the saved baseline database stat to the current
 * stat right before comparison in db_check_changes(). This simulates a faulty
 * "no metadata drift" result after a real database update and must trigger
 * WARNING.
 */
static Return test0013_8(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);
	create(char,path);

	struct stat stat_before_real_update = {0};
	struct stat stat_after_real_update = {0};

	const char *prepare_command = "cd ${TMPDIR}; "
	        "rm -f database1.db database1.db.backup; "
	        "rm -rf tests/examples/diff1_backup; "
	        "mv tests/examples/diffs/diff1 tests/examples/diff1_backup; "
	        "cp -a tests/examples/diff1_backup tests/examples/diffs/diff1;";
	const char *cleanup_command = "cd ${TMPDIR}; "
	        "rm -f database1.db database1.db.backup; "
	        "rm -rf tests/examples/diffs/diff1; "
	        "mv tests/examples/diff1_backup tests/examples/diffs/diff1;";
	const char *arguments = NULL;
	const char *command = NULL;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_TIMESTAMPS_WILL_BUMPED","false"));
	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED","false"));

	ASSERT(SUCCESS == external_call(prepare_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* First run: create DB in normal mode. */
	arguments = "--database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Make at least one real filesystem change so --update modifies DB. */
	command = "cd ${TMPDIR}; "
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Control check: real --update must modify database metadata. */
	ASSERT(SUCCESS == construct_path("database1.db",path));
	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat_before_real_update));

	command = "cd ${TMPDIR}; cp -a database1.db database1.db.backup;";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--update --database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0013_006_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == get_file_stat(getcstring(path),&stat_after_real_update));
	ASSERT(FAILURE == check_file_identity(&stat_before_real_update,&stat_after_real_update));

	/*
	 * Restore pre-update DB snapshot so the next run starts from the same state
	 * and tests only the simulation hook behavior.
	 */
	command = "cd ${TMPDIR}; rm -f database1.db; mv database1.db.backup database1.db;";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED","true"));
	arguments = "--update --database=database1.db tests/examples/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0013_006_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == set_environment_variable("PRECIZER_TEST_DB_FILE_STAT_WILL_BE_RESYNCED","false"));
	ASSERT(SUCCESS == external_call(cleanup_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(path);
	del(pattern);
	del(result);

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

	TEST(test0013_1,"The DB file should not be created…");
	TEST(test0013_2,"Dry run with checksums hashes files but keeps DB untouched…");
	TEST(test0013_3,"Invalid dry-run mode should return failure and print stderr error…");
	TEST(test0013_4,"The DB file should not be updated…");
	TEST(test0013_5,"Now run the same without simulation…");
	TEST(test0013_6,"Compare dry and real mode templates…");
	TEST(test0013_7,"Dry-run DB metadata drift should trigger internal warning path…");
	TEST(test0013_8,"Live update: force missing DB metadata drift and trigger warning…");

	RETURN_STATUS;
}
