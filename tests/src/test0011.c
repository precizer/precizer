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

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/diffs/diff1;"
	        "${BINDIR}/precizer --progress --database=database2.db tests/examples/diffs/diff2;"
	        "${BINDIR}/precizer --compare database1.db database2.db";

	// Create memory for the result
	MSTRUCT(mem_char,result);

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	char *pattern = NULL;

	const char *filename = "templates/0011_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db database2.db",SUCCESS,false,false));

	reset(&pattern);

	del_char(&result);

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
 * cp -par tests/examples/ tests/examples_backup
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

	// Preparation for tests
	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"cp -par tests/examples/ tests/examples_backup;",SUCCESS,false,false));

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/diffs/diff1";

	// Create memory for the result
	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0011_002_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --progress --database=database1.db tests/examples/diffs/diff1";

	filename = "templates/0011_002_2.txt";

	ASSERT(SUCCESS == execute_command(command,result,FAILURE,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean to use it iteratively
	reset(&pattern);
	del_char(&result);

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --progress --database=database1.db tests/examples/diffs/diff1;"
	        "echo -n '  ' >> tests/examples/diffs/diff1/1/AAA/BCB/CCC/a.txt;"
	        "touch tests/examples/diffs/diff1/1/AAA/BCB/CCC/c.txt;"
	        "rm tests/examples/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt;"
	        "${BINDIR}/precizer --update --progress --database=database1.db tests/examples/diffs/diff1";

	filename = "templates/0011_002_3.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	reset(&pattern);
	del_char(&result);

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

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --silent --update --progress --database=database1.db tests/examples/diffs/diff1;";

	// Create memory for the result
	MSTRUCT(mem_char,result);

	// Stdout output should be 0 characters
	ASSERT(result->length == 0);

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));

	del_char(&result);

	// Don't clean up test results to use on the next test

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

	const char *command = "export TESTING=false;cd ${TMPDIR};"
	        "${BINDIR}/precizer --verbose --update --progress --database=database1.db tests/examples/diffs/diff1";

	// Create memory for the result
	MSTRUCT(mem_char,result);

	char *pattern = NULL;

	const char *filename = "templates/0011_004_1.txt";

	ASSERT(SUCCESS == execute_command(command,result,SUCCESS,false,false));
	ASSERT(SUCCESS == get_file_content(filename,&pattern));
	ASSERT(SUCCESS == match_pattern(result->mem,pattern,filename));

	// Clean up test results
	reset(&pattern);
	del_char(&result);

	ASSERT(SUCCESS == external_call("cd ${TMPDIR};"
		"rm database1.db;"
		"rm -rf tests/examples/;"
		"mv tests/examples_backup/ tests/examples/",SUCCESS,false,false));

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

	const char *command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --maxdepth=0 tests/examples/4";

	const char *filename = "templates/0011_005_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	/* At the second stage, the --maxdepth=0 option is not used.
	   Therefore, all files that were not previously included
	   will be added to the database. */

	command = "export TESTING=true;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update tests/examples/4";

	filename = "templates/0011_005_2.txt";

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));;

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",SUCCESS,false,false));

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

	const char *command = "export TESTING=false;cd ${TMPDIR};"
	        "${BINDIR}/precizer --ignore=\"^diff1/1/.*\" tests/examples/diffs";

	const char *filename = "templates/0011_006_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	filename = "templates/0011_006_2.txt";

	command = "export TESTING=false;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update tests/examples/diffs";

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

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

	const char *command = "export TESTING=false;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update --db-clean-ignored --ignore=\"^diff1/1/.*\" --ignore=\"^diff2/1/.*\" tests/examples/diffs";

	const char *filename = "templates/0011_007.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",SUCCESS,false,false));

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

	const char *command = "cd ${TMPDIR};"
	        "${BINDIR}/precizer tests/examples/diffs";

	ASSERT(SUCCESS == execute_command(command,NULL,SUCCESS,true,true));

	command = "export TESTING=false;cd ${TMPDIR};"
	        "${BINDIR}/precizer --update"
	        " --db-clean-ignored --ignore=\"^.*/path2/.*\""
	        " --ignore=\"^diff2/.*\" --include=\"^diff2/1/AAA/ZAW/A/b/c/.*\""
	        " --include=\"^diff2/path1/AAA/ZAW/.*\" tests/examples/diffs";

	const char *filename = "templates/0011_008.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	if(replacement == NULL)
	{
		echo(STDERR,"ERROR: The environment variable DBNAME is not set\n");
		return(FAILURE);
	}

	ASSERT(SUCCESS == match_file_template(command,filename,template,replacement,0));

	ASSERT(SUCCESS == external_call("rm \"${TMPDIR}/${DBNAME}\"",SUCCESS,false,false));

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
