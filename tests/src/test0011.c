#include "sute.h"

/**
 * The Example 1 from README:
 * Now some tests could be running:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/fixtures/diffs/diff1
 * Stage 2. Adding:
 * precizer --progress --database=database2.db tests/fixtures/diffs/diff2
 * Final stage. Comparing:
 * precizer --compare database1.db database2.db
 */
static Return test0011_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,chunk,MEMORY_STRING);

	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_copy(result,chunk));

	arguments = "--progress --database=database2.db "
	        "tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	arguments = "--compare database1.db database2.db";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0011_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

	m_del(pattern);
	m_del(chunk);
	m_del(result);

	RETURN_STATUS;
}

/**
 * The Example 2 from README
 * Updating the database:
 * Stage 1. Adding:
 * precizer --progress --database=database1.db tests/fixtures/diffs/diff1
 * Stage 2. Reuse previous example once agan. The first try. The warning message.
 * precizer --progress --database=database1.db tests/fixtures/diffs/diff1
 * Stage 3. Run of database update without making actual changes to disk:
 * precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
 * Stage 4. Now let's make some changes:
 * # Backup
 * prepare_mutable_fixture("tests/fixtures/diffs/diff1")
 * # Modify a file
 * echo -n "  " >> tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt
 * # Add a new file by truncating the file with the target name
 * truncate_file_to_zero_size("tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/c.txt")
 * # Remove a file
 * delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt")
 * Stage 5. Run the precizer once again:
 * precizer --update --progress --database=database1.db tests/fixtures/diffs/diff1
 * Final stage. Recover from backup:
 * restore_mutable_fixture("tests/fixtures/diffs/diff1")
 */
static Return test0011_2(void)
{
	INITTEST;

	// Preparation for tests
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	const char *arguments = "--progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0011_002_1.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	arguments = "--progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	filename = "templates/0011_002_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	m_create(char,chunk,MEMORY_STRING);

	arguments = "--update --progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == m_copy(result,chunk));

	ASSERT(SUCCESS == add_string_to("  ","tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	// Create the file as empty without using shell touch
	ASSERT(SUCCESS == truncate_file_to_zero_size("tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/c.txt"));

	arguments = "--watch-timestamps --update --progress "
	        "--database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	filename = "templates/0011_002_3.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);
	m_del(chunk);

	// Don't clean up test results to use on the next test

	RETURN_STATUS;
}

/**
 * The Example 3 from README
 * Using the --silent mode. When this mode is enabled, the program suppresses
 * normal output after command execution. This makes sense when using the
 * program inside scripts. An exception is --compare: with --silent, compare
 * results remain
 * visible. Paths with differences are printed directly, and category headings
 * are kept only when more than one compare category is active
 * Let's add the --silent option to the previous example:
 *
 * precizer --silent --update --progress --database=database1.db tests/fixtures/diffs/diff1
 *
 *
 */
static Return test0011_3(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--silent --update --progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	// Verify that silent mode produced no stdout after command execution
	ASSERT(result->length == 0);

#if 0
	// Verify that silent mode produced no stdout after command execution
	if(result->length != 0)
	{
		echo(STDERR,"ERROR: In silent mode stdout must be empty\n");
		echo(STDERR,YELLOW "Output:\n>>" RESET "%s" YELLOW "<<\n" RESET,m_text(result));
		status = FAILURE;
	}
#endif

	call(m_del(result));

	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *filename = "templates/0011_003.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 *
 * The Example 4 from README
 * Additional information with --verbose mode
 *
 */
static Return test0011_4(void)
{
	INITTEST;

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);

	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0011_004_1.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--verbose --update --progress --database=database1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	m_del(pattern);
	m_del(result);

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	RETURN_STATUS;
}

/**
 *
 * The Example 5 from README
 * Disable recursion with --maxdepth=0 option
 *
 *
 */
static Return test0011_5(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--maxdepth=0 tests/fixtures/4";

	const char *filename = "templates/0011_005_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	/* At the second stage, the --maxdepth=0 option is not used.
	   Therefore, all files that were not previously included
	   will be added to the database. */

	arguments = "--update tests/fixtures/4";

	filename = "templates/0011_005_2.txt";

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == delete_path(replacement));

	RETURN_STATUS;
}

/**
 *
 * The Example 6 from README
 * Relative path to ignore with --ignore
 *
 *
 */
static Return test0011_6(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--ignore=\"^diff1/1/.*\" tests/fixtures/diffs";

	const char *filename = "templates/0011_006_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	filename = "templates/0011_006_2.txt";

	arguments = "--update tests/fixtures/diffs";

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
static Return test0011_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *arguments = "--update --db-drop-ignored"
	        " --ignore=\"^diff1/1/.*\""
	        " --ignore=\"^diff2/1/.*\" tests/fixtures/diffs";

	const char *filename = "templates/0011_007.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == delete_path(replacement));

	RETURN_STATUS;
}

/**
 *
 * The Example 8 from README
 * Using the --ignore options together with --include
 * Also covers the compare example that narrows the reported comparison scope
 *
 *
 */
static Return test0011_8(void)
{
	INITTEST;

	const char *arguments = "tests/fixtures/diffs";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	arguments = "--update"
	        " --progress"
	        " --ignore=\"^.*/path2/.*\""
	        " --ignore=\"^diff2/.*\""
	        " --include=\"^diff2/1/AAA/ZAW/A/b/c/.*\""
	        " --include=\"^diff2/path1/AAA/ZAW/.*\""
	        " --include=\"^diff1/path2/AAA/ZAW/A/b/c/a_file\\..*\""
	        " --db-drop-ignored"
	        " tests/fixtures/diffs";

	const char *filename = "templates/0011_008_1.txt";

	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	ASSERT(SUCCESS == delete_path(replacement));

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	arguments = "--progress --database=database1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--progress --database=database2.db tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--compare"
	        " --ignore=\"^(?:2|3|4)/.*\""
	        " --ignore=\"^path1/.*\""
	        " --ignore=\"^path2/.*\""
	        " --include=\"^2/AAA/BBB/CZC/a\\.txt$\""
	        " database1.db database2.db";

	filename = "templates/0011_008_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(pattern);
	m_del(result);

	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));

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

	TEST(test0011_1,"README Example 1 Adding and comparing");
	TEST(test0011_2,"README Example 2 Updating the data in DB");
	TEST(test0011_3,"README Example 3 --silent mode");
	TEST(test0011_4,"README Example 4 --verbose mode");
	TEST(test0011_5,"README Example 5 Disable recursion with --maxdepth");
	TEST(test0011_6,"README Example 6 Relative path to ignore with --ignore");
	TEST(test0011_7,"README Example 7 Multiple regexp for ignoring");
	TEST(test0011_8,"README Example 8 The --ignore options together with --include, including the compare-scope example");
	SUTE(test0030,"README Examples 9 & 10: --lock-checksum with --rehash-locked and --watch-timestamps");
	SUTE(test0029,"README Example 11: Testing how the application behaves with inaccessible files");

	RETURN_STATUS;
}
