#include "sute.h"

/**
 * The Example 1 from README:
 * Now some tests could be running:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/examples/diffs/diff2
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return test0011_1_readme(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Create memory for the result
	create(char,result);
	create(char,chunk);

	const char *arguments = "--progress --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--progress --database=database2.db tests/examples/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	create(char,pattern);

	const char *filename = "templates/0011_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db database2.db",COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(chunk);
	del(result);

	RETURN_STATUS;
}

/**
 * The Example 2 from README
 * Updating the database:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 2. Reuse previous example once agan. The first try. The warning message.
 * precizer --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 3. Run of database update without making actual changes to disk:
 * precizer --update --progress --database=database1.db tests/examples/diffs/diff1
 * Stage 4. Now let's make some changes:
 * # Backup
 * cp -a tests/examples/ tests/examples_backup
 * # Modify a file
 * echo -n "  " >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt
 * # Add a new file
 * touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt
 * # Remove a file
 * rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt
 * Stage 5. Run the precizer once again:
 * precizer --update --progress --database=database1.db tests/examples/diffs/diff1
 * Final stage. Recover from backup:
 * rm -rf tests/examples/
 * mv tests/examples_backup/ tests/examples/
 */
static Return test0011_2_readme(void)
{
	INITTEST;

	const char *command = "cd ${TMPDIR};"
	        "mv tests/examples/diffs/ tests/examples_backup/;"
	        "cp -a tests/examples_backup/ tests/examples/diffs/;";

	// Preparation for tests
	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--progress --database=database1.db tests/examples/diffs/diff1";

	// Create memory for the result
	create(char,result);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	create(char,pattern);

	const char *filename = "templates/0011_002_1.txt";

	ASSERT(SUCCESS == runit(arguments,result,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	arguments = "--progress --database=database1.db tests/examples/diffs/diff1";

	filename = "templates/0011_002_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	create(char,chunk);

	arguments = "--update --progress --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	command = "cd ${TMPDIR};"
	        "echo -n '  ' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt;"
	        "touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt;"
	        "rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt";

	ASSERT(SUCCESS == external_call(command,COMPLETED,ALLOW_BOTH));

	arguments = "--watch-timestamps --update --progress --database=database1.db tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	filename = "templates/0011_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);
	del(chunk);

	// Don't clean up test results to use on the next test

	RETURN_STATUS;
}

/**
 * The Example 3 from README
 * Using the --silent mode. When this mode is enabled, the program does not display
 * anything on the screen. This makes sense when using the program inside scripts.
 * Let's add the --silent option to the previous example:
 *
 * precizer --silent --update --progress --database=database1.db tests/examples/diffs/diff1
 *
 *
 */
static Return test0011_3_readme(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == runit("--silent --update --progress --database=database1.db tests/examples/diffs/diff1",result,COMPLETED,ALLOW_BOTH));

	// Verify that silent mode produced no stdout after command execution
	if(result->length > 0)
	{
		echo(STDERR,"ERROR: In silent mode stdout must be empty\n");
		echo(STDERR,YELLOW "Output:\n>>" RESET "%s" YELLOW "<<\n" RESET,getcstring(result));
		status = FAILURE;
	}

	call(del(result));

	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *filename = "templates/0011_003.txt";

	ASSERT(SUCCESS == runit("--silent --update --progress --database=database1.db tests/examples/diffs/diff1",result,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * The Example 4 from README
 * Additional information with --verbose mode
 *
 */
static Return test0011_4_readme(void)
{
	INITTEST;

	// Create memory for the result
	create(char,result);

	create(char,pattern);

	const char *filename = "templates/0011_004_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == runit("--verbose --update --progress --database=database1.db tests/examples/diffs/diff1",result,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	del(pattern);
	del(result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR} && "
		"rm database1.db && "
		"rm -rf tests/examples/diffs/ && "
		"mv tests/examples_backup/ tests/examples/diffs/",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * The Example 5 from README
 * Disable recursion with --maxdepth=0 option
 *
 *
 */
static Return test0011_5_readme(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--maxdepth=0 tests/examples/4";

	const char *filename = "templates/0011_005_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	/* At the second stage, the --maxdepth=0 option is not used.
	   Therefore, all files that were not previously included
	   will be added to the database. */

	arguments = "--update tests/examples/4";

	filename = "templates/0011_005_2.txt";

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * The Example 6 from README
 * Relative path to ignore with --ignore
 *
 *
 */
static Return test0011_6_readme(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--ignore=\"^diff1/1/.*\" tests/examples/diffs";

	const char *filename = "templates/0011_006_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	filename = "templates/0011_006_2.txt";

	arguments = "--update tests/examples/diffs";

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	RETURN_STATUS;
}

/**
 *
 * The Example 7 from README
 * Multiple regular expressions for ignoring can be specified
 * using many --ignore options
 *
 */
static Return test0011_7_readme(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--update --db-clean-ignored"
	        " --ignore=\"^diff1/1/.*\""
	        " --ignore=\"^diff2/1/.*\""
	        " tests/examples/diffs";

	const char *filename = "templates/0011_007.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * The Example 8 from README
 * Using the --ignore option(s) together with --include
 *
 *
 */
static Return test0011_8_readme(void)
{
	INITTEST;

	const char *arguments = NULL;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("tests/examples/diffs",NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	arguments = "--update"
	        " --db-clean-ignored"
	        " --ignore=\"^.*/path2/.*\""
	        " --ignore=\"^diff2/.*\""
	        " --include=\"^diff2/1/AAA/ZAW/A/b/c/.*\""
	        " --include=\"^diff2/path1/AAA/ZAW/.*\""
	        " tests/examples/diffs";

	const char *filename = "templates/0011_008.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * User's Manual and examples from README test set
 *
 */
Return test0011(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	INITTEST;

	TEST(test0011_1_readme,"README example 1 Adding and comparing…");
	TEST(test0011_2_readme,"README example 2 Updating the data in DB…");
	TEST(test0011_3_readme,"README example 3 --silent mode…");
	TEST(test0011_4_readme,"README example 4 --verbose mode…");
	TEST(test0011_5_readme,"README example 5 Disable recursion with --maxdepth…");
	TEST(test0011_6_readme,"README example 6 Relative path to ignore with --ignore…");
	TEST(test0011_7_readme,"README example 7 Multiple regexp for ignoring…");
	TEST(test0011_8_readme,"README example 8 The --ignore option(s) together with --include…");

	RETURN_STATUS;
}
