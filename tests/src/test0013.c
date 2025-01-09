#include "sute.h"

/**
 * @brief Constructs full file path by combining TMPDIR environment variable with filename
 *
 * @param[in] filename Name of the file to append to TMPDIR path
 * @param[out] full_path Pointer to string pointer that will store the allocated path
 * @return Return Status of the operation
 */
Return construct_path(
	const char *filename,
	char       **full_path
){
	Return status = SUCCESS;
	const char *tmp_dir = NULL;
	size_t path_len = 0;

	if(SUCCESS == status)
	{
		if(NULL == filename || NULL == full_path)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		tmp_dir = getenv("TMPDIR");

		if(NULL == tmp_dir)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		path_len = strlen(tmp_dir) + strlen(filename) + 2;   // +2 for '/' and '\0'
		*full_path = (char *)malloc(path_len);

		if(NULL == *full_path)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int print_result = snprintf(*full_path,path_len,"%s/%s",tmp_dir,filename);

		if(print_result < 0 || (size_t)print_result >= path_len)
		{
			free(*full_path);
			*full_path = NULL;
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * The db file should not be created in the Dry Run mode
 */
static Return dry_run_mode_1_test(void){
	Return status = SUCCESS;

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "./precizer --dry-run --database=database1.db tests/examples/diffs/diff1";

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

	free(path);

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
	        "cp -r tests/examples/ tests/examples_backup/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,SUCCESS,false,false));

	command = "export TESTING=true;cd ${TMPDIR};"
	        "./precizer --database=database1.db tests/examples/diffs/diff1";

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
	        "./precizer --dry-run --update --database=database1.db tests/examples/diffs/diff1";

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
	        "./precizer --dry-run --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	const char *filename = "templates/0013_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	// Dry Run mode permanent deletion of all ignored file
	// references from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "./precizer --db-clean-ignored --ignore=\"^1/AAA/ZAW/*\""
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	command = "export TESTING=true;cd ${TMPDIR};"
	        "./precizer --db-clean-ignored --ignore=\"^path2/AAA/ZAW/*\""
	        " --update --dry-run --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	ASSERT(SUCCESS == get_file_stat(path,&stat2));

	ASSERT(SUCCESS == check_file_identity(&stat1,&stat2));

	free(path);

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
	        "./precizer --database=database1.db tests/examples/diffs/diff1";

	// Preparation for tests
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"cp -r tests/examples/ tests/examples_backup/;",SUCCESS,false,false));

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
	        "cp database1.db database1.db.backup;"
	        "./precizer --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	const char *filename = "templates/0013_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	// Real live mode permanent deletion of all ignored file
	// references from the database
	command = "export TESTING=true;cd ${TMPDIR};"
	        "cp database1.db.backup database1.db;"
	        "./precizer --db-clean-ignored --ignore=\"^1/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "cp database1.db.backup database1.db;"
	        "./precizer --db-clean-ignored --ignore=\"^path2/AAA/ZAW/*\""
	        " --update --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	#if 0
	echo(STDOUT,"%s\n",result->mem);
	#endif

	filename = "templates/0013_003_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	free(pattern);
	pattern = NULL;

	del_char(&result);

	free(path);

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db;"
		"rm -rf tests/examples/;"
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

	RETURN_STATUS;
}

// Main test runner
Return test0013(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(dry_run_mode_1_test,"The DB file should not be created…");
	TEST(dry_run_mode_2_test,"The DB file should not be updated…");
//	TEST(no_dry_run_mode_3_test,"Now run in live mode without simulation…");

	RETURN_STATUS;
}
