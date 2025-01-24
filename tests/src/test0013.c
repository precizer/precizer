#include "sute.h"

/**
 * The db file should not be created in the Dry Run mode
 */
static Return dry_run_mode_1_test(void){
	Return status = SUCCESS;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --dry-run --database=database1.db tests/examples/diffs/diff1";

	MSTRUCT(mem_char,result);

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	del_char(&result);

	// Does file exists or not
	const char *db_filename = "database1.db";
	char *path = NULL;
	bool file_exists = false;

	ASSERT(SUCCESS == construct_path(db_filename,&path));

	ASSERT(SUCCESS == check_file_exists(&file_exists,path));

	reset(&path);

	// Should not be exists
	ASSERT(file_exists == false);

	RETURN_STATUS;
}

/**
 * The db file should not be updated in the Dry Run mode
 */
static Return dry_run_mode_2_test(void){
	Return status = SUCCESS;
	// Create memory for the result
	MSTRUCT(mem_char,result);
	struct stat stat1;
	struct stat stat2;
	char *pattern = NULL;

	const char *command = "cd ${TMPDIR};"
	        "cp -pr tests/examples/ tests/examples_backup/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"Path: %s\n",path);
	#endif

	char *path = NULL;
	const char *db_filename = "database1.db";

	ASSERT(SUCCESS == construct_path(db_filename,&path));

	ASSERT(SUCCESS == get_file_stat(path,&stat1));

	command = "export TESTING=true;cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" // Remove
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Modify
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;" // New file
	        "${BINDIR}/precizer --dry-run --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Compare against the sample. A message should be displayed indicating
	// that the --db-clean-ignored option must be specified for permanent
	// removal of ignored files from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --dry-run --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	const char *filename = "templates/0013_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Dry Run mode permanent deletion of all ignored file
	// references from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --db-clean-ignored --ignore=\"^1/AAA/ZAW/*\""
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --db-clean-ignored --ignore=\"^path2/AAA/ZAW/*\""
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	reset(&path);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db;"
		"rm -rf tests/examples/;"
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

	RETURN_STATUS;
}

/**
 * Everything that was previously executed in Dry Run mode
 * will now be tested in live mode without simulation, and
 * the results will be compared against each other
 */
static Return no_dry_run_mode_3_test(void){
	Return status = SUCCESS;
	// Create memory for the result
	MSTRUCT(mem_char,result);
	char *path = NULL;
	char *pattern = NULL;
	const char *db_file_name = "database1.db";
	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --database=database1.db tests/examples/diffs/diff1";

	// Preparation for tests
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"cp -pr tests/examples/ tests/examples_backup/;",SUCCESS,false,false));

	ASSERT(SUCCESS == construct_path(db_file_name,&path));

	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"Path: %s\n",path);
	#endif

	command = "export TESTING=true;cd ${TMPDIR};"
	        "rm tests/examples/diffs/diff1/2/AAA/BBB/CZC/a.txt;" // Remove
	        "echo -n AFAKDSJ >> tests/examples/diffs/diff1/1/AAA/ZAW/D/e/f/b_file.txt;" // Modify
	        "echo -n WNEURHGO > tests/examples/diffs/diff1/2/AAA/BBB/CZC/b.txt;"; // New file

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	del_char(&result);

	// Compare against the sample. A message should be displayed indicating
	// that the --db-clean-ignored option must be specified for permanent
	// removal of ignored files from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p database1.db database1.db.backup;"
	        "${BINDIR}/precizer --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	const char *filename = "templates/0013_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	// Real live mode permanent deletion of all ignored file
	// references from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "cp -p database1.db.backup database1.db;"
	        "${BINDIR}/precizer --db-clean-ignored --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "mv database1.db.backup database1.db;"
	        "${BINDIR}/precizer --db-clean-ignored --ignore=\"^path2/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_003_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);

	del_char(&result);

	reset(&path);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db;"
		"rm -rf tests/examples/;"
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

	RETURN_STATUS;
}

Return compare_dry_and_real_4_test(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *text1 = NULL;
	char *text2 = NULL;
	char *diff = NULL;
	char *pattern = NULL;
	const char *filename = NULL;

	/* 0013 002 1 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_1.txt",&text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_1.txt",&text2));

	ASSERT(SUCCESS == compare_strings(&diff,text1,text2));

	filename = "templates/0013_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(diff,pattern,filename));

	reset(&text1);
	reset(&text2);
	reset(&diff);
	reset(&pattern);

	/* 0013 002 2 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_2.txt",&text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_2.txt",&text2));

	ASSERT(SUCCESS == compare_strings(&diff,text1,text2));

	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	ASSERT(SUCCESS == match_pattern(diff,pattern,filename));

	reset(&text1);
	reset(&text2);
	reset(&diff);
	reset(&pattern);

	/* 0013 002 3 */
	ASSERT(SUCCESS == get_file_content("templates/0013_002_3.txt",&text1));

	ASSERT(SUCCESS == get_file_content("templates/0013_003_3.txt",&text2));

	ASSERT(SUCCESS == compare_strings(&diff,text1,text2));

	// _2 is not a mistake. 2 and 3 are equals
	filename = "templates/0013_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(diff,pattern,filename));

	reset(&text1);
	reset(&text2);
	reset(&diff);
	reset(&pattern);

	RETURN_STATUS;
}

/**
 *
 * Dry Run mode testing
 *
 */
Return test0013(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(dry_run_mode_1_test,"The DB file should not be created…");
	TEST(dry_run_mode_2_test,"The DB file should not be updated…");
	TEST(no_dry_run_mode_3_test,"Now run the same without simulation…");
	TEST(compare_dry_and_real_4_test,"Compare dry and real mode templates…");

	RETURN_STATUS;
}
