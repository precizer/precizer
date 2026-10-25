#include "test_librational_all.h"

/**
 * @brief Emit machine records, technical errors, and hidden service messages
 *
 * Both a standalone MACHINE mode and a mode containing every ordinary category
 * must admit only MACHINE and ERROR payloads without verbose or testing prefixes
 */
static void emit_machine_mode_messages(void)
{
	const LOGMODES modes[] = {MACHINE,MACHINE|EVERY|ERROR};

	for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
	{
		rational_logger_mode = modes[i];
		slog(MACHINE,"machine plain\n");
		slog(MACHINE|UNDECOR,"machine undecorated\n");
		slog(MACHINE|EVERY,"machine mixed flags\n");
		slog(ERROR,"technical error\n");
		slog(ERROR|UNDECOR,"technical error undecorated\n");
		slog(EVERY|ERROR,"mixed error flags\n");
		slog(EVERY,"unexpected every message\n");
		slog(REGULAR,"unexpected regular message\n");
		slog(VERBOSE,"unexpected verbose message\n");
		slog(TESTING,"unexpected testing message\n");
		slog(TRACE,"unexpected trace message\n");
	}
}

/**
 * @brief Emit ordinary logger categories while machine-only messages stay hidden
 *
 * Fixed source metadata leaves only the verbose timestamp variable in the
 * complete output template. A disabled logger must also suppress machine records
 */
static void emit_ordinary_mode_messages(void)
{
	const struct {
		LOGMODES mode;
		LOGMODES level;
		const char *message;
	} cases[] = {
		{REGULAR,REGULAR,"regular"},
		{VERBOSE,VERBOSE,"verbose"},
		{TESTING,TESTING,"testing"},
		{EVERY,EVERY|UNDECOR,"every undecorated"},
		{ERROR,ERROR,"ordinary error"},
		{0,REGULAR,"unexpected disabled message"}
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
	{
		rational_logger_mode = cases[i].mode;
		slog(MACHINE,"unexpected machine message\n");
		slog(MACHINE|UNDECOR,"unexpected undecorated machine message\n");
		rational_logger(cases[i].level,"logger_matrix.c",42U,
			"emit_ordinary_mode_messages","%s\n",cases[i].message);
	}
}

/**
 * @brief Emit messages that exercise the silent-mode visibility exception
 *
 * SILENT suppresses ordinary machine records and errors. VISIBLE_IN_SILENT
 * admits any category without diagnostic prefixes, including EVERY and ERROR
 */
static void emit_silent_mode_messages(void)
{
	const LOGMODES modes[] = {SILENT,SILENT|MACHINE,SILENT|MACHINE|EVERY|ERROR};

	for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
	{
		rational_logger_mode = modes[i];
		slog(MACHINE,"unexpected silent machine message\n");
		slog(ERROR,"unexpected silent error\n");
		slog(EVERY,"unexpected silent every message\n");
		slog(MACHINE|VISIBLE_IN_SILENT,"visible machine\n");
		slog(ERROR|VISIBLE_IN_SILENT,"visible error\n");
		slog(EVERY|VISIBLE_IN_SILENT,"visible every\n");
		slog(VISIBLE_IN_SILENT,"visible without category\n");
	}
}

/**
 * @brief Check complete logger output against a file template and require empty stderr
 *
 * @param[in] template_path Path to the complete stdout golden expression relative to $TMPDIR/tests
 * @param emit_output Callback that emits the logger scenarios
 * @return SUCCESS when both streams satisfy their expectations, otherwise FAILURE
 */
static Return check_logger_output(
	const char *template_path,
	void (     *emit_output )(void))
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const RATIONAL_COLOR_MODE initial_color_mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	rational_color_mode = COLOR_MODE_NEVER;
	run(function_capture(emit_output,captured_stdout,captured_stderr));
	run(read_file_content(template_path,pattern));
	run(match_pattern(captured_stdout,pattern,template_path));
	ASSERT(captured_stderr->length == 0U);

	/* Restore the surrounding suite's logger settings after any result */
	rational_logger_mode = initial_logger_mode;
	rational_color_mode = initial_color_mode;
	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	provide(status);
}

/**
 * @brief Check that MACHINE admits machine records and technical errors only
 * @return Return status for machine visibility and diagnostic prefixes
 */
static Return test_librational_0007_1(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_logger_output("templates/librational/0007_001.txt",emit_machine_mode_messages));
	RETURN_STATUS;
}

/**
 * @brief Check ordinary logger behavior and suppression of machine-only records
 * @return Return status for compatibility with ordinary and disabled modes
 */
static Return test_librational_0007_2(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_logger_output("templates/librational/0007_002.txt",emit_ordinary_mode_messages));
	RETURN_STATUS;
}

/**
 * @brief Check SILENT precedence and its explicit visibility exception
 * @return Return status for silent filtering and undecorated visible payloads
 */
static Return test_librational_0007_3(void)
{
	INITTEST;
	ASSERT(SUCCESS == check_logger_output("templates/librational/0007_003.txt",emit_silent_mode_messages));
	RETURN_STATUS;
}

/**
 * @brief Check MACHINE logger visibility independently of application output settings
 * @return Return status for machine, ordinary, and silent logger modes
 */
Return test_librational_0007(void)
{
	INITTEST;

	TEST(test_librational_0007_1,"MACHINE admits records and errors without service output");
	TEST(test_librational_0007_2,"Ordinary logger modes keep their output and hide machine records");
	TEST(test_librational_0007_3,"SILENT keeps precedence and explicit message visibility");

	RETURN_STATUS;
}
