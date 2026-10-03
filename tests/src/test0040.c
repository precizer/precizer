#include "sute.h"

#define DRY_RUN_ARGUMENTS "--dry-run --database=0040.db tests/fixtures/diffs/diff1/2"

struct color_environment_backup {
	char *no_color;
	char *term;
	bool snapshot_is_valid;
};

static const char *remembered_value = "remembered";
static LOGMODES remembered_log_level = REGULAR | UNDECOR | REMEMBER;

/**
 * @brief Write one decorated message through the REMEMBER logger path
 */
static void capture_remembered_message(void)
{
	slog(remembered_log_level,
		"@{bold}@{red}%s %zu@{reset}\n",
		remembered_value,
		(size_t)7U);
}

/**
 * @brief Save environment values that affect automatic color output
 *
 * @param backup Zero-initialized storage for duplicated environment values
 * @return SUCCESS when the values were saved, otherwise FAILURE
 */
static Return save_color_environment(struct color_environment_backup *backup)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *no_color = getenv("NO_COLOR");
	const char *term = getenv("TERM");

	/* Copy existing values so an unset variable remains distinct from an empty one */
	if(no_color != NULL)
	{
		backup->no_color = strdup(no_color);
		ASSERT(backup->no_color != NULL);
	}

	if(SUCCESS == status && term != NULL)
	{
		backup->term = strdup(term);
		ASSERT(backup->term != NULL);
	}

	/* An incomplete snapshot must never replace the original environment */
	backup->snapshot_is_valid = SUCCESS == status;

	deliver(status);
}

/**
 * @brief Restore environment values saved before a color output test
 *
 * @param backup Saved environment values to restore and release
 * @return SUCCESS when the environment was restored, otherwise FAILURE
 */
static Return restore_color_environment(struct color_environment_backup *backup)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Attempt both restorations before checking their results */
	if(backup->snapshot_is_valid == true)
	{
		int no_color_result = 0;
		int term_result = 0;

		if(backup->no_color != NULL)
		{
			no_color_result = setenv("NO_COLOR",backup->no_color,1);
		} else {
			no_color_result = unsetenv("NO_COLOR");
		}

		if(backup->term != NULL)
		{
			term_result = setenv("TERM",backup->term,1);
		} else {
			term_result = unsetenv("TERM");
		}

		ASSERT(no_color_result == 0);
		ASSERT(term_result == 0);
	}

	/* Release complete or partial snapshots after any restoration result */
	free(backup->term);
	free(backup->no_color);
	memset(backup,0,sizeof(*backup));

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
static Return check_last_remembered_message(
	sqlite3    *database,
	const char *expected_message,
	size_t     expected_length)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *statement = NULL;

	/* Read the most recently stored message from the real SQLite table */
	ASSERT(SQLITE_OK == sqlite3_prepare_v2(database,
		"SELECT message FROM temp.remember_history ORDER BY id DESC LIMIT 1;",
		-1,&statement,NULL));
	ASSERT(SQLITE_ROW == sqlite3_step(statement));

	/* Require the exact byte count and text, including prefixes and the newline */
	if(SUCCESS == status)
	{
		const unsigned char *stored_message = sqlite3_column_text(statement,0);
		ASSERT(stored_message != NULL);
		ASSERT(sqlite3_column_bytes(statement,0) == (int)expected_length);
		ASSERT(memcmp(stored_message,expected_message,expected_length) == 0);
	}

	/* Release the statement even when a query or content check failed */
	const int finalize_result = sqlite3_finalize(statement);
	ASSERT(finalize_result == SQLITE_OK);

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
static Return assert_output(
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

	/* Run with test diagnostics enabled and collect both output streams */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,COMPLETED,STDERR_ALLOW));

	/* Check the selected mode, complete output, styling, and absence of errors */
	ASSERT(SUCCESS == get_file_content(stdout_pattern_file,stdout_pattern));
	ASSERT(SUCCESS == replace_placeholder(stdout_pattern,"%COLOR_MODE%",expected_color_mode));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));
	ASSERT((strchr(m_text(stdout_result),'\033') != NULL) == colors_expected);
	ASSERT(stderr_result->length == 0U);

	/* Release capture buffers and the template after any assertion result */
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
static Return assert_argument_error(
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

	/* Require the rejected command to finish with the expected failure status */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == runit(arguments,stdout_result,stderr_result,FAILURE,STDERR_ALLOW));

	/* Compare application diagnostics and argument-parser errors in full */
	ASSERT(SUCCESS == get_file_content(stdout_pattern_file,stdout_pattern));
	ASSERT(SUCCESS == match_pattern(stdout_result,stdout_pattern,stdout_pattern_file));
	ASSERT(SUCCESS == get_file_content(stderr_pattern_file,stderr_pattern));
	ASSERT(SUCCESS == match_pattern(stderr_result,stderr_pattern,stderr_pattern_file));

	/* Release both templates and captured streams even after a failed check */
	call(m_del(stderr_pattern));
	call(m_del(stdout_pattern));
	call(m_del(stderr_result));
	call(m_del(stdout_result));

	deliver(status);
}

/**
 * @brief Check the test runner's forced color default without a color argument
 *
 * The runner sets TESTITALL_TEST_ENV_COLOR_MODE=always so captured golden
 * output retains styling. This case checks that test setting; the application's
 * ordinary automatic default is checked separately in test0040_7()
 *
 * @return Return describing success or failure
 */
static Return test0040_1(void)
{
	INITTEST;

	/* Leave --color unspecified so the test-only override determines the mode */
	ASSERT(SUCCESS == assert_output(
		DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	RETURN_STATUS;
}

/**
 * @brief Check all long color mode arguments with captured output
 *
 * Each --color value must appear in the parsed configuration and produce the
 * expected output. Always retains styling; never and auto omit it from the
 * redirected stream
 *
 * @return Return describing success or failure
 */
static Return test0040_2(void)
{
	INITTEST;

	/* Forced styling must survive output redirection */
	ASSERT(SUCCESS == assert_output(
		"--color=always " DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	/* Explicitly disabling styling must produce plain text */
	ASSERT(SUCCESS == assert_output(
		"--color=never " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"never",
		false));

	/* Auto must select its own mode while keeping redirected output plain */
	ASSERT(SUCCESS == assert_output(
		"--color=auto " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	RETURN_STATUS;
}

/**
 * @brief Check all short color mode arguments with captured output
 *
 * Each -S value must select the same mode and output as its --color equivalent
 *
 * @return Return describing success or failure
 */
static Return test0040_3(void)
{
	INITTEST;

	/* Forced styling must survive output redirection */
	ASSERT(SUCCESS == assert_output(
		"-S always " DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	/* Explicitly disabling styling must produce plain text */
	ASSERT(SUCCESS == assert_output(
		"-S never " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"never",
		false));

	/* Auto must select its own mode while keeping redirected output plain */
	ASSERT(SUCCESS == assert_output(
		"-S auto " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	RETURN_STATUS;
}

/**
 * @brief Check that forced color output is preserved with NO_COLOR set
 *
 * Captured auto output would be plain even without NO_COLOR because stdout
 * is redirected. The always run must retain styling even when NO_COLOR is
 * present with an empty value
 *
 * @return Return describing success or failure
 */
static Return test0040_4(void)
{
	INITTEST;
	struct color_environment_backup environment_backup = {0};

	/* Save the environment and set NO_COLOR to an explicitly empty value */
	ASSERT(SUCCESS == save_color_environment(&environment_backup));
	ASSERT(0 == setenv("NO_COLOR","",1));
	ASSERT(0 == setenv("TERM","xterm",1));

	/* Redirected auto output remains plain under this environment setting */
	ASSERT(SUCCESS == assert_output(
		"--color=auto " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	/* Always must preserve styling despite the presence of NO_COLOR */
	ASSERT(SUCCESS == assert_output(
		"--color=always " DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	/* Restore the original environment even when an assertion failed */
	call(restore_color_environment(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check that forced color output is preserved with TERM=dumb
 *
 * Captured auto output would be plain even without TERM=dumb because stdout
 * is redirected. The always run must retain styling even when TERM identifies
 * a dumb terminal
 *
 * @return Return describing success or failure
 */
static Return test0040_5(void)
{
	INITTEST;
	struct color_environment_backup environment_backup = {0};

	/* Save the environment and isolate TERM=dumb from NO_COLOR */
	ASSERT(SUCCESS == save_color_environment(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","dumb",1));

	/* Redirected auto output remains plain under this environment setting */
	ASSERT(SUCCESS == assert_output(
		"--color=auto " DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	/* Always must preserve styling despite TERM=dumb */
	ASSERT(SUCCESS == assert_output(
		"--color=always " DRY_RUN_ARGUMENTS,
		"templates/0040_001.txt",
		"always",
		true));

	/* Restore the original environment even when an assertion failed */
	call(restore_color_environment(&environment_backup));

	RETURN_STATUS;
}

/**
 * @brief Check immediate message styling and plain-text storage in SQLite
 *
 * Messages logged with REMEMBER are displayed immediately and stored for later
 * reporting. In always mode, markup adds terminal styling only to the displayed
 * copy; the stored row contains plain text. Markup supplied through a formatted
 * argument is also expanded, while raw terminal control bytes become visible
 * hexadecimal escapes. In never mode, the displayed message and stored row
 * must both contain the same plain text
 *
 * @return Return describing success or failure
 */
static Return test0040_6(void)
{
	INITTEST;

	/* Preserve the logger settings used by the surrounding test suite */
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);

	/* Markup in the formatted argument must expand, while raw ANSI bytes must
	   remain visible as escaped text in both the output and the stored message */
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
	Config *initial_config = config;
	Config remember_config = {0};

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	/* Give the real REMEMBER callback an isolated in-memory SQLite table */
	ASSERT(SQLITE_OK == sqlite3_open(":memory:",&remember_config.db));
	config = &remember_config;
	ASSERT(SQLITE_OK == sqlite3_exec(remember_config.db,create_table_sql,NULL,NULL,NULL));

	/* Capture a styled message with a TESTING prefix */
	rational_logger_mode = TESTING;
	rational_color_mode = COLOR_MODE_ALWAYS;
	remembered_log_level = TESTING | REMEMBER;
	remembered_value = styled_value;
	ASSERT(SUCCESS == function_capture(
		capture_remembered_message,
		captured_stdout,
		captured_stderr));

	/* Check expanded markup and escaped raw controls in the displayed line */
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_styled_output));
	ASSERT(captured_stderr->length == 0U);

	/* SQLite must receive the same text and prefix without terminal styling */
	ASSERT(SUCCESS == check_last_remembered_message(
		remember_config.db,
		expected_stored_styled_message,
		sizeof(expected_stored_styled_message) - 1U));

	/* Disable styling and capture an ordinary message without a log prefix */
	rational_logger_mode = REGULAR;
	rational_color_mode = COLOR_MODE_NEVER;
	remembered_log_level = REGULAR | UNDECOR | REMEMBER;
	remembered_value = "never";
	ASSERT(SUCCESS == function_capture(
		capture_remembered_message,
		captured_stdout,
		captured_stderr));

	/* Both the immediate output and stored row must contain plain text */
	ASSERT(0 == strcmp(m_text(captured_stdout),expected_plain_output));
	ASSERT(NULL == strchr(m_text(captured_stdout),'\033'));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(SUCCESS == check_last_remembered_message(
		remember_config.db,
		expected_plain_output,
		sizeof(expected_plain_output) - 1U));

	/* Restore shared state and close the isolated database after any result */
	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	remembered_log_level = REGULAR | UNDECOR | REMEMBER;
	remembered_value = "remembered";

	config = initial_config;
	const int close_result = sqlite3_close(remember_config.db);
	ASSERT(close_result == SQLITE_OK);

	/* Release both captured output streams */
	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check default automatic color output without the test-only override
 *
 * Removing TESTITALL_TEST_ENV_COLOR_MODE exposes the application's normal
 * auto default. NO_COLOR is absent and TERM is xterm, so the captured output
 * must be plain because it is redirected to a file. The runner mode and all
 * changed environment values are restored after the check
 *
 * @return Return describing success or failure
 */
static Return test0040_7(void)
{
	INITTEST;
	const enum run_mode initial_run_mode = testitall_runit_mode;
	struct color_environment_backup environment_backup = {0};
	const char *test_color_override = getenv("TESTITALL_TEST_ENV_COLOR_MODE");
	const bool test_color_override_was_set = test_color_override != NULL;
	char *saved_test_color_override = NULL;

	/* Copy the override before unsetenv invalidates its environment storage */
	if(test_color_override_was_set == true)
	{
		saved_test_color_override = strdup(test_color_override);
		ASSERT(saved_test_color_override != NULL);
	}

	/* Remove forced styling and environment settings that could disable auto */
	ASSERT(SUCCESS == save_color_environment(&environment_backup));
	ASSERT(0 == unsetenv("NO_COLOR"));
	ASSERT(0 == setenv("TERM","xterm",1));
	ASSERT(0 == unsetenv("TESTITALL_TEST_ENV_COLOR_MODE"));

	/* Use the standalone executable to check its default color behavior.
	   With stdout redirected to a file, auto must produce plain text */
	testitall_runit_mode = EXTERNAL_CALL;
	ASSERT(SUCCESS == assert_output(
		DRY_RUN_ARGUMENTS,
		"templates/0040_002.txt",
		"auto",
		false));

	/* Restore shared test state even when an assertion failed */
	testitall_runit_mode = initial_run_mode;

	if(saved_test_color_override != NULL)
	{
		const int restore_result = setenv("TESTITALL_TEST_ENV_COLOR_MODE",saved_test_color_override,1);
		ASSERT(restore_result == 0);
	} else if(test_color_override_was_set == false){
		const int restore_result = unsetenv("TESTITALL_TEST_ENV_COLOR_MODE");
		ASSERT(restore_result == 0);
	}

	/* Release the override copy and restore the remaining environment values */
	free(saved_test_color_override);
	call(restore_color_environment(&environment_backup));

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

	/* Both option spellings must reject an unsupported mode name */
	ASSERT(SUCCESS == assert_argument_error(
		"--color=invalid-value",
		"templates/0040_004.txt"));
	ASSERT(SUCCESS == assert_argument_error(
		"-S invalid-value",
		"templates/0040_004.txt"));

	/* An explicitly empty value must also be rejected by both spellings */
	ASSERT(SUCCESS == assert_argument_error(
		"--color=",
		"templates/0040_005.txt"));
	ASSERT(SUCCESS == assert_argument_error(
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

	/* Check the parser diagnostic for a long option with no following value */
	ASSERT(SUCCESS == assert_argument_error(
		"--color",
		"templates/0040_006.txt"));

	/* Check the corresponding diagnostic for the short option */
	ASSERT(SUCCESS == assert_argument_error(
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

	/* Verbose diagnostics must name always and retain styled messages */
	ASSERT(SUCCESS == assert_output(
		"--verbose --color=always " DRY_RUN_ARGUMENTS,
		"templates/0040_008.txt",
		"always",
		true));

	/* Verbose diagnostics must name never and contain no terminal styling */
	ASSERT(SUCCESS == assert_output(
		"--verbose --color=never " DRY_RUN_ARGUMENTS,
		"templates/0040_009.txt",
		"never",
		false));

	/* Auto must be reported distinctly while redirected output stays plain */
	ASSERT(SUCCESS == assert_output(
		"--verbose --color=auto " DRY_RUN_ARGUMENTS,
		"templates/0040_009.txt",
		"auto",
		false));

	RETURN_STATUS;
}

#undef DRY_RUN_ARGUMENTS

/**
 * @brief Check color output modes and plain-text storage of remembered messages
 *
 * Successful command-line cases scan a small directory fixture in dry-run mode
 * and compare the captured output with full templates. They check --color and
 * -S, the test runner's forced color setting, the application's automatic
 * default, forced styling with NO_COLOR or TERM=dumb, and verbose diagnostics.
 * Invalid, empty, and missing option values must produce the expected errors.
 *
 * A separate logger case stores messages in a real temporary SQLite table.
 * Displayed messages may contain styling, while stored messages must remain
 * plain text with raw terminal control bytes escaped
 *
 * @return SUCCESS when all color output and message-storage scenarios pass
 */
Return test0040(void)
{
	INITTEST;

	/* Check the runner default, both option spellings, and forced styling */
	TEST(test0040_1,"Test runs preserve colored dry-run output by default");
	TEST(test0040_2,"Long color options control captured dry-run output");
	TEST(test0040_3,"Short color options control captured dry-run output");
	TEST(test0040_4,"Forced color output is preserved with NO_COLOR set");
	TEST(test0040_5,"Forced color output is preserved with TERM=dumb");

	/* Compare immediate message formatting with the bytes stored in SQLite */
	TEST(test0040_6,"REMEMBER stores plain text while immediate output uses color markup");

	/* Check the application default without the runner override */
	TEST(test0040_7,"Default auto mode omits color from captured output without the test override");

	/* Verify rejected values and the mode reported in verbose diagnostics */
	TEST(test0040_8,"Color options reject unsupported and empty values");
	TEST(test0040_9,"Color options reject missing values");
	TEST(test0040_10,"Verbose diagnostics report the selected color mode");

	RETURN_STATUS;
}
