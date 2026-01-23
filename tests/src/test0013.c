#include "sute.h"

/**
 * The db file should not be created in the Dry Run mode
 */
static Return dry_run_mode_1_test(void)
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
 * The db file should not be updated in the Dry Run mode
 */
static Return dry_run_mode_2_test(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);
	struct stat stat1;
	struct stat stat2;
	create(char,pattern);
	create(char,chunk);

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

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

	arguments = "--dry-run --update --database=database1.db "
	        "tests/examples/diffs/diff1";

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
	// that the --db-clean-ignored option must be specified for permanent
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

	arguments = "--db-clean-ignored --ignore=\"^1/AAA/ZAW/.*\" --update "
	        "--dry-run --database=database1.db tests/examples/diffs/diff1";

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

	arguments = "--watch-timestamps --db-clean-ignored "
	        "--ignore=\"^path2/AAA/ZAW/.*\" --update --dry-run "
	        "--database=database1.db tests/examples/diffs/diff1";

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
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Everything that was previously executed in Dry Run mode
 * will now be tested in live mode without simulation, and
 * the results will be compared against each other
 */
static Return no_dry_run_mode_3_test(void)
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
	        "cp -a tests/examples/diffs/ tests/examples_backup/;";
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
	// that the --db-clean-ignored option must be specified for permanent
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

	arguments = "--db-clean-ignored --ignore=\"^1/AAA/ZAW/.*\" --update "
	        "--database=database1.db tests/examples/diffs/diff1";

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

	arguments = "--watch-timestamps --db-clean-ignored "
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
	        "rm -rf tests/examples/diffs/ && "
	        "mv tests/examples_backup/ tests/examples/diffs/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

Return compare_dry_and_real_4_test(void)
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
 *
 * Dry Run mode testing
 *
 */
Return test0013(void)
{
	INITTEST;

	TEST(dry_run_mode_1_test,"The DB file should not be created…");
	TEST(dry_run_mode_2_test,"The DB file should not be updated…");
	TEST(no_dry_run_mode_3_test,"Now run the same without simulation…");
	TEST(compare_dry_and_real_4_test,"Compare dry and real mode templates…");

	RETURN_STATUS;
}
