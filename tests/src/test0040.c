#include "sute.h"

#define TEST0040_DRY_RUN_ARGUMENTS "--dry-run --database=0040.db tests/fixtures/diffs/diff1/2"

struct test0040_color_environment_backup {
	char *no_color;
	char *term;
	bool no_color_was_set;
	bool term_was_set;
	bool snapshot_is_valid;
};

static const char *test0040_remembered_value = "remembered";
static LOGMODES test0040_remembered_log_level = REGULAR | UNDECOR | REMEMBER;

/**
 * @brief Write one decorated message through the REMEMBER logger path
 */
static void test0040_capture_remembered_message(void)
{
	slog(test0040_remembered_log_level,
		"@{bold}@{red}%s %zu@{reset}\n",
		test0040_remembered_value,
		(size_t)7U);
}

/**
 * @brief Save environment values that affect automatic color output
 *
 * @param backup Destination for duplicated environment values
 * @return SUCCESS when the values were saved, otherwise FAILURE
 */
static Return test0040_save_color_environment(struct test0040_color_environment_backup *backup)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *no_color = NULL;
	const char *term = NULL;

	if(backup == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		memset(backup,0,sizeof(*backup));
		no_color = getenv("NO_COLOR");
		term = getenv("TERM");
	}

	if(SUCCESS == status && no_color != NULL)
	{
		backup->no_color = strdup(no_color);

		if(backup->no_color == NULL)
		{
			status = FAILURE;
		} else {
			backup->no_color_was_set = true;
		}
	}

	if(SUCCESS == status && term != NULL)
	{
		backup->term = strdup(term);

		if(backup->term == NULL)
		{
			status = FAILURE;
		} else {
			backup->term_was_set = true;
		}
	}

	if(SUCCESS == status)
	{
		backup->snapshot_is_valid = true;
	}

	deliver(status);
}

/**
 * @brief Restore environment values saved before a color output test
 *
 * @param backup Saved environment values to restore and release
 * @return SUCCESS when the environment was restored, otherwise FAILURE
 */
static Return test0040_restore_color_environment(struct test0040_color_environment_backup *backup)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(backup == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && true == backup->snapshot_is_valid)
	{
		if(true == backup->no_color_was_set)
		{
			if(0 != setenv("NO_COLOR",backup->no_color,1))
			{
				status = FAILURE;
			}
		} else if(0 != unsetenv("NO_COLOR")){
			status = FAILURE;
		}

		if(true == backup->term_was_set)
		{
			if(0 != setenv("TERM",backup->term,1))
			{
				status = FAILURE;
			}
		} else if(0 != unsetenv("TERM")){
			status = FAILURE;
		}
	}

	if(backup != NULL)
	{
		free(backup->term);
		free(backup->no_color);
		memset(backup,0,sizeof(*backup));
	}

	deliver(status);
}

/**
 * @brief Check the most recent message stored in the real REMEMBER table
 *
 * @param[in] database Database containing the temporary remember_history table
 * @param[in] expected_message Exact plain message expected in the last row
 * @param expected_length Number of bytes in @p expected_message
 * @return SUCCESS when the latest row contains the expected bytes, otherwise FAILURE
 */
static Return test0040_check_last_remembered_message(
	sqlite3    *database,
	const char *expected_message,
	size_t     expected_length)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *statement = NULL;
	const unsigned char *stored_message = NULL;
	int stored_length = 0;

	if(database == NULL || expected_message == NULL || expected_length > INT_MAX)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(database,
		"SELECT message FROM temp.remember_history ORDER BY id DESC LIMIT 1;",
		-1,
		&statement,
		NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_ROW != sqlite3_step(statement))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		stored_message = sqlite3_column_text(statement,0);
		stored_length = sqlite3_column_bytes(statement,0);

		if(stored_message == NULL
		        || stored_length != (int)expected_length
		        || 0 != memcmp(stored_message,expected_message,expected_length))
		{
			status = FAILURE;
		}
	}

	if(SQLITE_OK != sqlite3_finalize(statement))
	{
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Run a deterministic dry run and compare its complete captured output
 *
 * @param arguments Complete dry-run command-line arguments
 * @param stdout_pattern_file Full stdout template for the selected color mode
 * @param expected_color_mode Exact mode name to substitute into the diagnostic template
 * @param colors_expected True when captured stdout must contain terminal color sequences
 * @return SUCCESS when stdout matches and stderr is empty, otherwise FAILURE
 */
static Return test0040_assert_output(
	const char *arguments,
	const char *stdout_pattern_file,
	const char *expected_color_mode,
	const bool colors_expected)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	if(arguments == NULL || stdout_pattern_file == NULL || expected_color_mode == NULL)
	{
		status = FAILURE;
	}

	run(set_environment_variable("TESTING","true"));
	run(runit(arguments,stdout_result,stderr_result,COMPLETED,STDERR_ALLOW));
	run(get_file_content(stdout_pattern_file,stdout_pattern));
	run(replace_placeholder(stdout_pattern,"%COLOR_MODE%",expected_color_mode));
	run(match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));

	if(SUCCESS == status)
	{
		const bool colors_were_written = strchr(m_text(stdout_result),'\033') != NULL;

		if(colors_were_written != colors_expected)
		{
			status = FAILURE;
		}
	}

	run(m_copy_literal(stderr_pattern,"\\A\\Z"));
	run(match_pattern(stderr_result,stderr_pattern,NULL));

	call(m_del(stderr_pattern));
	call(m_del(stdout_pattern));
	call(m_del(stderr_result));
	call(m_del(stdout_result));

	deliver(status);
}

/**
 * @brief Check a rejected color argument against complete output templates
 *
 * @param arguments Command-line arguments containing an invalid or missing color value
 * @param stderr_pattern_file Full stderr template for the expected argument error
 * @return SUCCESS when the application exits with failure and both streams match
 */
static Return test0040_assert_argument_error(
	const char *arguments,
	const char *stderr_pattern_file)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *stdout_pattern_file = "templates/0040_003.txt";

	m_create(char,stdout_result,MEMORY_STRING);
	m_create(char,stderr_result,MEMORY_STRING);
	m_create(char,stdout_pattern,MEMORY_STRING);
	m_create(char,stderr_pattern,MEMORY_STRING);

	run(set_environment_variable("TESTING","true"));
	run(runit(arguments,stdout_result,stderr_result,FAILURE,STDERR_ALLOW));
	run(get_file_content(stdout_pattern_file,stdout_pattern));
	run(match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));
	run(get_file_content(stderr_pattern_file,stderr_pattern));
	run(match_pattern(stderr_result,stderr_pattern,stderr_pattern_file));

	call(m_del(stderr_pattern));
	call(m_del(stdout_pattern));
	call(m_del(stderr_result));
	call(m_del(stdout_result));

	deliver(status);
}

/**
 * @brief Check the test runner's colored default output
 *
 * @return Return describing success or failure
 */
static Return test0040_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_output(
		TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	RETURN_STATUS;
}

/**
 * @brief Check all long color mode arguments with captured output
 *
 * @return Return describing success or failure
 */
static Return test0040_2(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_output(
		"--color=always " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=never " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"never",
		false));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=auto " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	RETURN_STATUS;
}

/**
 * @brief Check all short color mode arguments with captured output
 *
 * @return Return describing success or failure
 */
static Return test0040_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_output(
		"-S always " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));
	ASSERT(SUCCESS == test0040_assert_output(
		"-S never " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"never",
		false));
	ASSERT(SUCCESS == test0040_assert_output(
		"-S auto " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	RETURN_STATUS;
}

/**
 * @brief Check NO_COLOR priority for automatic and forced color modes
 *
 * @return Return describing success or failure
 */
static Return test0040_4(void)
{
	INITTEST;
	struct test0040_color_environment_backup environment_backup = {0};

	ASSERT(SUCCESS == test0040_save_color_environment(&environment_backup));
	ASSERT(0 == setenv("NO_COLOR","",1));
	ASSERT(0 == setenv("TERM","xterm",1));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=auto " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=always " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	call(test0040_restore_color_environment(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check TERM=dumb priority for automatic and forced color modes
 *
 * @return Return describing success or failure
 */
static Return test0040_5(void)
{
	INITTEST;
	struct test0040_color_environment_backup environment_backup = {0};

	ASSERT(SUCCESS == test0040_save_color_environment(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","dumb",1));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=auto " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));
	ASSERT(SUCCESS == test0040_assert_output(
		"--color=always " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	call(test0040_restore_color_environment(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check that REMEMBER writes decorated terminal output but stores plain text
 *
 * @return Return describing success or failure
 */
static Return test0040_6(void)
{
	INITTEST;

	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	static const char styled_value[] =
	        "always @{green}dynamic@{reset} " BOLD "inside" RESET "\033[2Jblocked";
	static const char expected_styled_output[] = "TESTING:" BOLD RED "always "
	        GREEN "dynamic" RESET " \\x1B[1minside\\x1B[0m\\x1B[2Jblocked 7"
	        RESET "\n";
	static const char expected_stored_styled_message[] =
	        "TESTING:always dynamic \\x1B[1minside\\x1B[0m\\x1B[2Jblocked 7\n";
	static const char expected_plain_output[] = "never 7\n";
	static const char create_table_sql[] =
	        "CREATE TEMP TABLE remember_history ("
	        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
	        "message TEXT NOT NULL"
	        ");";
	sqlite3 *initial_database = NULL;
	sqlite3 *remember_database = NULL;
	bool database_was_replaced = false;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	ASSERT(config != NULL);

	if(SUCCESS == status)
	{
		initial_database = config->db;
	}

	ASSERT(SQLITE_OK == sqlite3_open(":memory:",&remember_database));

	if(SUCCESS == status)
	{
		config->db = remember_database;
		database_was_replaced = true;
	}

	ASSERT(SQLITE_OK == sqlite3_exec(remember_database,create_table_sql,NULL,NULL,NULL));

	rational_logger_mode = TESTING;
	rational_color_mode = COLOR_MODE_ALWAYS;
	test0040_remembered_log_level = TESTING | REMEMBER;
	test0040_remembered_value = styled_value;
	ASSERT(SUCCESS == function_capture(
		test0040_capture_remembered_message,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_styled_output));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == test0040_check_last_remembered_message(
		remember_database,
		expected_stored_styled_message,
		sizeof(expected_stored_styled_message) - 1U));

	rational_logger_mode = REGULAR;
	rational_color_mode = COLOR_MODE_NEVER;
	test0040_remembered_log_level = REGULAR | UNDECOR | REMEMBER;
	test0040_remembered_value = "never";
	ASSERT(SUCCESS == function_capture(
		test0040_capture_remembered_message,
		captured_stdout,
		captured_stderr));
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_plain_output));
	ASSERT(NULL == strchr(m_text(captured_stdout),'\033'));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == test0040_check_last_remembered_message(
		remember_database,
		expected_plain_output,
		sizeof(expected_plain_output) - 1U));

	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	test0040_remembered_log_level = REGULAR | UNDECOR | REMEMBER;
	test0040_remembered_value = "remembered";

	if(database_was_replaced == true)
	{
		config->db = initial_database;
	}

	if(remember_database != NULL && SQLITE_OK != sqlite3_close(remember_database))
	{
		status = FAILURE;
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check default automatic color output without the test-only override
 *
 * The external executable uses its normal default while runit captures its
 * output in files. The runner mode and all changed environment values are
 * restored after the check
 *
 * @return Return describing success or failure
 */
static Return test0040_7(void)
{
	INITTEST;
	const enum run_mode initial_run_mode = testitall_runit_mode;
	struct test0040_color_environment_backup environment_backup = {0};
	const char *test_color_override = getenv("TESTITALL_TEST_ENV_COLOR_MODE");
	const bool test_color_override_was_set = test_color_override != NULL;
	char *saved_test_color_override = NULL;

	if(test_color_override_was_set == true)
	{
		saved_test_color_override = strdup(test_color_override);
		ASSERT(saved_test_color_override != NULL);
	}

	ASSERT(SUCCESS == test0040_save_color_environment(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));
	ASSERT(0 == unsetenv("TESTITALL_TEST_ENV_COLOR_MODE"));

	/* Use the standalone executable to check its default color behavior */
	testitall_runit_mode = EXTERNAL_CALL;
	ASSERT(SUCCESS == test0040_assert_output(
		TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	/* Restore shared test state even when an assertion failed */
	testitall_runit_mode = initial_run_mode;

	if(saved_test_color_override != NULL)
	{
		if(0 != setenv("TESTITALL_TEST_ENV_COLOR_MODE",saved_test_color_override,1))
		{
			status = FAILURE;
		}
	} else if(test_color_override_was_set == false){
		if(0 != unsetenv("TESTITALL_TEST_ENV_COLOR_MODE"))
		{
			status = FAILURE;
		}
	}

	free(saved_test_color_override);
	call(test0040_restore_color_environment(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check that long and short color options reject unsupported and empty values
 *
 * @return Return describing success or failure
 */
static Return test0040_8(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_argument_error(
		"--color=invalid-value",
		"templates/0040_004.txt"));
	ASSERT(SUCCESS == test0040_assert_argument_error(
		"-S invalid-value",
		"templates/0040_004.txt"));
	ASSERT(SUCCESS == test0040_assert_argument_error(
		"--color=",
		"templates/0040_005.txt"));
	ASSERT(SUCCESS == test0040_assert_argument_error(
		"-S ''",
		"templates/0040_005.txt"));

	RETURN_STATUS;
}

/**
 * @brief Check that long and short color options require a value
 *
 * @return Return describing success or failure
 */
static Return test0040_9(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_argument_error(
		"--color",
		"templates/0040_006.txt"));
	ASSERT(SUCCESS == test0040_assert_argument_error(
		"-S",
		"templates/0040_007.txt"));

	RETURN_STATUS;
}

/**
 * @brief Check that verbose diagnostics report each selected color mode
 *
 * @return Return describing success or failure
 */
static Return test0040_10(void)
{
	INITTEST;

	ASSERT(SUCCESS == test0040_assert_output(
		"--verbose --color=always " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_008.txt",
		"always",
		true));
	ASSERT(SUCCESS == test0040_assert_output(
		"--verbose --color=never " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_009.txt",
		"never",
		false));
	ASSERT(SUCCESS == test0040_assert_output(
		"--verbose --color=auto " TEST0040_DRY_RUN_ARGUMENTS,
		"templates/0040_009.txt",
		"auto",
		false));

	RETURN_STATUS;
}

#undef TEST0040_DRY_RUN_ARGUMENTS

/**
 * @brief Run command-line color output tests
 *
 * @return SUCCESS when all color output scenarios pass
 */
Return test0040(void)
{
	INITTEST;

	TEST(test0040_1,"Test runs preserve colored dry-run output by default");
	TEST(test0040_2,"Long color options control captured dry-run output");
	TEST(test0040_3,"Short color options control captured dry-run output");
	TEST(test0040_4,"NO_COLOR disables automatic color but not --color=always");
	TEST(test0040_5,"TERM=dumb disables automatic color but not --color=always");
	TEST(test0040_6,"REMEMBER stores plain text while immediate output uses color markup");
	TEST(test0040_7,"Default auto mode omits color from captured output without the test override");
	TEST(test0040_8,"Color options reject unsupported and empty values");
	TEST(test0040_9,"Color options reject missing values");
	TEST(test0040_10,"Verbose diagnostics report the selected color mode");

	RETURN_STATUS;
}
