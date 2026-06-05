#include "sute.h"

/**
 * @brief Verify informational-mode output against the expected stdout template
 *
 * @param[in] arguments CLI arguments to pass to the application
 * @param[in] stdout_pattern_file Template file for expected stdout
 * @return Return status code
 */
static Return assert_information_mode_output(
	const char *arguments,
	const char *stdout_pattern_file)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	if(arguments == NULL)
	{
		status = FAILURE;
	}

	if((SUCCESS & status) && stdout_pattern_file == NULL)
	{
		status = FAILURE;
	}

	run(set_environment_variable("TESTING","true"));

	run(runit(arguments,stdout_result,stderr_result,COMPLETED,STDERR_ALLOW));

	run(get_file_content(stdout_pattern_file,stdout_pattern));
	run(match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));

	run(m_copy_literal(stderr_pattern,"\\A\\Z"));
	run(match_pattern(stderr_result,stderr_pattern));

	call(m_del(stderr_pattern));
	call(m_del(stdout_pattern));
	call(m_del(stderr_result));
	call(m_del(stdout_result));

	deliver(status);
}

/**
 *
 * Check the name of the database created by default.
 * Does it really comply with to the "hostname.db" template
 *
 */
Return test0003_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--progress tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	arguments = "--progress --database=database2.db "
	        "tests/fixtures/diffs/diff2";

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Get the output of the application
	arguments = "--compare $DBNAME database2.db";

	const char *filename = "templates/0003_001.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	ASSERT(SUCCESS == delete_path(replacement));
	ASSERT(SUCCESS == delete_path("database2.db"));

	RETURN_STATUS;
}

/**
 *
 * Running the application with no arguments at all
 *
 */
Return test0003_2(void)
{
	INITTEST;

	const char *arguments = "";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,COMPLETED,STDERR_ALLOW));

	const char *filename = "templates/0003_002.txt";

	ASSERT(SUCCESS == get_file_content(filename,stderr_pattern));

	// Match stderr against the pattern
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern,filename));

	filename = "templates/0003_003.txt";

	ASSERT(SUCCESS == get_file_content(filename,stdout_pattern));

	// Match stdout against the pattern
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,filename));

	// Clean to use it iteratively
	m_del(stderr_pattern);
	m_del(stdout_pattern);
	m_del(stderr_result);
	m_del(stdout_result);

	RETURN_STATUS;

}

/**
 *
 * Information modes should complete successfully without PATH
 *
 */
Return test0003_3(void)
{
	INITTEST;
	const char *help_template = "templates/0003_004.txt";
	const char *usage_template = "templates/0003_005.txt";
	const char *version_template = "templates/0003_006.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS & assert_information_mode_output("--help",help_template));
	ASSERT(SUCCESS & assert_information_mode_output("-h",help_template));
	ASSERT(SUCCESS & assert_information_mode_output("--usage",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("-z",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("--version",version_template));
	ASSERT(SUCCESS & assert_information_mode_output("--version --version",version_template));

	RETURN_STATUS;
}

/**
 *
 * Information modes should ignore PATH and not run the main workflow
 *
 */
Return test0003_4(void)
{
	INITTEST;
	const char *help_template = "templates/0003_004.txt";
	const char *usage_template = "templates/0003_005.txt";
	const char *version_template = "templates/0003_006.txt";

	ASSERT(SUCCESS & assert_information_mode_output("--help /definitely/nonexistent/path",help_template));
	ASSERT(SUCCESS & assert_information_mode_output("-h /definitely/nonexistent/path",help_template));
	ASSERT(SUCCESS & assert_information_mode_output("--usage /definitely/nonexistent/path",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("-z /definitely/nonexistent/path",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("--version /definitely/nonexistent/path",version_template));
	ASSERT(SUCCESS & assert_information_mode_output("--compare --help",help_template));
	ASSERT(SUCCESS & assert_information_mode_output("--compare --usage",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("--compare -z",usage_template));
	ASSERT(SUCCESS & assert_information_mode_output("--compare --version",version_template));
	ASSERT(SUCCESS & assert_information_mode_output("--help --version",help_template));

	RETURN_STATUS;
}

Return test0003(void)
{
	INITTEST;

	TEST(test0003_1,"Comply default DB name to \"hostname.db\" template");
	TEST(test0003_2,"Running the application with no arguments at all");
	TEST(test0003_3,"Information modes without PATH return success");
	TEST(test0003_4,"Information modes ignore PATH and stop early");

	RETURN_STATUS;
}
