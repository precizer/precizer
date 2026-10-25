#include "sute.h"

/**
 * @brief Stop startup when a read error follows valid configuration settings
 *
 * The fixture represents the prefix read before an I/O failure. The ferror
 * mock reports that failure after inih applies the prefix. Complete output
 * matching rejects any traversal output, and the database must remain absent
 *
 * @return Return status for startup failure without a database being created
 */
static Return test0046_1(void)
{
	INITTEST;
	const struct argument_test_case cases[] = {
		{"--config=tests/templates/0046_001.conf",FAILURE,"templates/0003_013_2.txt","templates/0046_001.txt"},
		{0}
	};

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == expect_file_access_from_root(".","0046_001.db",F_OK,FILE_NOT_FOUND));

	mocks_ferror_reset();
	mocks_ferror_set_target_suffix("0046_001.conf");
	ASSERT(SUCCESS == run_and_match_templates(cases,NULL,NULL));
	ASSERT(mocks_ferror_call_count() == 1U);
	mocks_ferror_reset();

	ASSERT(SUCCESS == expect_file_access_from_root(".","0046_001.db",F_OK,FILE_NOT_FOUND));
	call(delete_path_if_present("0046_001.db"));
	RETURN_STATUS;
}

/**
 * @brief Check retained settings while the read-error diagnostic is captured
 * @return SUCCESS when reading fails after applying the configuration prefix
 */
static Return check_settings_after_read_error(void)
{
	INITTEST;
	char program_name[] = "testitall";
	struct argp_state state = {.name = program_name,.err_stream = stderr,.flags = ARGP_NO_EXIT};
	bool has_entries = false;
	m_create(char,path,MEMORY_STRING);

	ASSERT(SUCCESS == construct_path("tests/templates/0046_002.conf",path));
	ASSERT(FAILURE == read_config_file(m_text(path),true,&has_entries,&state));
	ASSERT(has_entries == true);
	ASSERT(config->config_file_error_line == -1);
	/* A valid prefix must remain applied when the final stream check reports EIO */
	ASSERT(config->roots.length == 1U);
	memory *roots = m_data(memory,conf(roots));
	ASSERT(strcmp(m_text(&roots[0]),"settings_read_before_failure") == 0);

	call(m_del(path));
	deliver(status);
}

/**
 * @brief Preserve settings read before a stream error and check its full diagnostic
 * @return Return status for retained settings, the failure trace, and the read-error message
 */
static Return test0046_2(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	config = &test_config;
	ASSERT(SUCCESS == init_config_and_check_output());
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);

	ASSERT(SUCCESS == get_file_content("templates/0046_002.txt",pattern));
	ASSERT(SUCCESS == get_file_content("templates/0045_034.txt",stdout_pattern));
	mocks_ferror_reset();
	mocks_ferror_set_target_suffix("0046_002.conf");
	ASSERT(SUCCESS == match_function_output(m_text(stdout_pattern),m_text(pattern),check_settings_after_read_error));
	ASSERT(mocks_ferror_call_count() == 1U);
	mocks_ferror_reset();

	/* Restore the runner's configuration and output policy after any test result */
	free_config();
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	call(m_del(stdout_pattern));
	call(m_del(pattern));
	RETURN_STATUS;
}

/**
 * @brief Check configuration read failures during reading and application startup
 * @return Return status for retained settings and startup failure before scanning
 */
Return test0046(void)
{
	INITTEST;

	TEST(test0046_1,"Read errors after configuration settings stop before scanning");
	TEST(test0046_2,"Settings read before a configuration stream error remain applied");

	RETURN_STATUS;
}
