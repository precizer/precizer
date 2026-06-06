#include "test_librational_all.h"
#include <errno.h>

static const char expected_report_stderr_pattern[] =
        "\\A"
        "ERROR: [^\\n]*src/test_librational_0004\\.c:report_test:\\d+ Memory reallocation failed with bytes 10 Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
        "ERROR: [^\\n]*src/test_librational_0004\\.c:report_test:\\d+ Buffer overflow at position 42 with value overflow Errno: [^\\n]+ \\(errno: [0-9]+\\)\\Z";

/* Regex fragments for slog() output fields that change between source layouts and test runs */
#define SLOG_TIME_PATTERN "[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}:[0-9]{3} "
#define SLOG_SOURCE_PATTERN "[^\\n]*src/test_librational_0004\\.c:[0-9]+:slog_test:"
#define SLOG_DECORATED_PATTERN SLOG_TIME_PATTERN SLOG_SOURCE_PATTERN

static const char expected_slog_stdout_pattern[] =
        "\\A"
        "All available combinations:\n"
        "REGULAR\n"
        "VERBOSE\n"
        "TESTING\n"
        "SILENT\n"
        "REGULAR\\|VERBOSE\n"
        "REGULAR\\|TESTING\n"
        "VERBOSE\\|TESTING\n"
        "REGULAR\\|VERBOSE\\|TESTING\n"
        "ERROR\n"
        "UNDECOR\n"
        "EVERY\\|UNDECOR\n"
        "ERROR\\|UNDECOR\n"
        "VISIBLE_IN_SILENT\n"
        "Mode: REGULAR\n"
        "1\\.  Must print:true\n"
        "2\\. Won't print:\n"
        "3\\. Won't print:\n"
        "4\\.  Must print:ERROR: true\n"
        "Mode: VERBOSE\n"
        "5\\. Won't print:\n"
        "6\\.  Must print:" SLOG_DECORATED_PATTERN "true\n"
        "7\\. Won't print:\n"
        "8\\.  Must print:" SLOG_DECORATED_PATTERN "ERROR:true\n"
        "Mode: TESTING\n"
        "9\\.  Won't print:\n"
        "10\\. Won't print:\n"
        "11\\.  Must print:TESTING:true\n"
        "12\\.  Must print:ERROR:true\n"
        "Mode: SILENT\n"
        "13\\. Won't print:\n"
        "14\\. Won't print:\n"
        "15\\. Won't print:\n"
        "16\\. Won't print:\n"
        "Mode: REGULAR \\| VERBOSE\n"
        "17\\.  Must print:true\n"
        "18\\.  Must print:" SLOG_DECORATED_PATTERN "true\n"
        "19\\. Won't print:\n"
        "20\\.  Must print:" SLOG_DECORATED_PATTERN "ERROR: true\n"
        "Mode: REGULAR \\| TESTING\n"
        "21\\.  Must print:true\n"
        "22\\. Won't print:\n"
        "23\\.  Must print:TESTING:true\n"
        "24\\.  Must print:ERROR: true\n"
        "Mode: VERBOSE \\| TESTING\n"
        "25\\. Won't print:\n"
        "26\\.  Must print:" SLOG_DECORATED_PATTERN "true\n"
        "27\\.  Must print:TESTING:true\n"
        "28\\.  Must print:" SLOG_DECORATED_PATTERN "ERROR:true\n"
        "Mode: REGULAR \\| VERBOSE \\| TESTING\n"
        "29\\. Must print:true\n"
        "30\\. Must print:" SLOG_DECORATED_PATTERN "true\n"
        "31\\. Must print:TESTING:true\n"
        "32\\. Must print:" SLOG_DECORATED_PATTERN "ERROR: true\n"
        "Mode: ERROR\n"
        "33\\. Won't print:\n"
        "34\\. Won't print:\n"
        "35\\. Won't print:\n"
        "36\\.  Must print:ERROR: true\n"
        "Mode: REGULAR \\| VERBOSE \\| TESTING \\| ERROR\n"
        "37\\. Must print no prefixes:\\|true\\|\n"
        "38\\. Must print no ERROR prefix:\\|true\\|\n"
        "Mode: VERBOSE\n"
        "39\\. Must print no time/file/line/func:\\|true\\|\n"
        "Mode: TESTING\n"
        "40\\. Must print no TESTING prefix:\\|true\\|\n"
        "Mode: REGULAR\n"
        "41\\. Must not print \\(VERBOSE not enabled\\):\\|\\|\n"
        "Mode: SILENT\n"
        "42\\. Must print in SILENT without prefixes:\\|true\\|\n"
        "43\\. Must print no ERROR prefix in SILENT:\\|true\\|\\Z";

static const char expected_serp_stderr_pattern[] =
        "\\A"
        "ERROR: Failed to open file \\[File: [^\\n]*src/test_librational_0004\\.c, Function: serp_case\\] Errno: \\(2\\) No such file or directory\\Z";

#ifndef EVIL_EMPIRE_OS
static const char expected_write_fallback_stderr_pattern[] =
        "\\AERROR: Failed to write error message\n\\Z";

static const char expected_logger_time_failure_stdout_pattern[] =
        "\\A"
        " [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:gettimeofday failure\n"
        " [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:localtime_r failure\n"
        " [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:snprintf failure\\Z";
#endif

/**
 * @brief Emit two report() messages with controlled errno values
 */
static void report_test(void)
{
	errno = ENOMEM;

	/* report() must include the formatted message and decoded errno */
	report("Memory reallocation failed with bytes %d",10);

	errno = EOVERFLOW;

	/* The second call verifies that several printf arguments are preserved */
	report("Buffer overflow at position %d with value %s",42,"overflow");
}

/**
 * @brief Emit the complete slog() mode matrix used by the strict stdout pattern
 */
static void slog_test(void)
{
	printf("All available combinations:\n");
	printf("%s\n",rational_convert(REGULAR));
	printf("%s\n",rational_convert(VERBOSE));
	printf("%s\n",rational_convert(TESTING));
	printf("%s\n",rational_convert(SILENT));
	printf("%s\n",rational_convert(REGULAR|VERBOSE));
	printf("%s\n",rational_convert(REGULAR|TESTING));
	printf("%s\n",rational_convert(VERBOSE|TESTING));
	printf("%s\n",rational_convert(REGULAR|VERBOSE|TESTING));
	printf("%s\n",rational_convert(ERROR));
	printf("%s\n",rational_convert(UNDECOR));
	printf("%s\n",rational_convert(EVERY|UNDECOR));
	printf("%s\n",rational_convert(ERROR|UNDECOR));
	printf("%s\n",rational_convert(VISIBLE_IN_SILENT));

	/* Test REGULAR mode combinations */
	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("1.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("2. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("3. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("4.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE mode combinations */
	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("5. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("6.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("7. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("8.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test TESTING mode combinations */
	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("9.  Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("10. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("11.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("12.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test SILENT mode combinations */
	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("13. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("14. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("15. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("16. Won't print:");   slog(ERROR,"but printed!"); printf("\n");

	/* Test REGULAR|VERBOSE combinations */
	rational_logger_mode = REGULAR|VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("17.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("18.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("19. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("20.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|TESTING combinations */
	rational_logger_mode = REGULAR|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("21.  Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("22. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("23.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("24.  Must print:"); slog(ERROR,"true"); printf("\n");

	/* Test VERBOSE|TESTING combinations */
	rational_logger_mode = VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("25. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("26.  Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("27.  Must print:"); slog(TESTING,"true"); printf("\n");
	printf("28.  Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test REGULAR|VERBOSE|TESTING combinations */
	rational_logger_mode = REGULAR|VERBOSE|TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("29. Must print:"); slog(REGULAR,"true"); printf("\n");
	printf("30. Must print:"); slog(VERBOSE,"true"); printf("\n");
	printf("31. Must print:"); slog(TESTING,"true"); printf("\n");
	printf("32. Must print:");   slog(ERROR,"true"); printf("\n");

	/* Test ERROR mode combinations */
	rational_logger_mode = ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("33. Won't print:"); slog(REGULAR,"but printed!"); printf("\n");
	printf("34. Won't print:"); slog(VERBOSE,"but printed!"); printf("\n");
	printf("35. Won't print:"); slog(TESTING,"but printed!"); printf("\n");
	printf("36.  Must print:");   slog(ERROR,"true"); printf("\n");

	/*
	 * Test UNDECOR flag: suppress logger prefixes (TESTING:, time/file/line/func, ERROR:)
	 * The output between the '|' markers should contain only the message payload
	 */
	rational_logger_mode = EVERY|ERROR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("37. Must print no prefixes:|"); slog(EVERY|UNDECOR,"true"); printf("|\n");
	printf("38. Must print no ERROR prefix:|"); slog(ERROR|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = VERBOSE;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("39. Must print no time/file/line/func:|"); slog(VERBOSE|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = TESTING;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("40. Must print no TESTING prefix:|"); slog(TESTING|UNDECOR,"true"); printf("|\n");

	rational_logger_mode = REGULAR;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("41. Must not print (VERBOSE not enabled):|"); slog(VERBOSE|UNDECOR,"but printed!"); printf("|\n");

	rational_logger_mode = SILENT;
	printf("Mode: %s\n",rational_reconvert(rational_logger_mode));
	printf("42. Must print in SILENT without prefixes:|"); slog(EVERY|VISIBLE_IN_SILENT,"true"); printf("|\n");
	printf("43. Must print no ERROR prefix in SILENT:|"); slog(ERROR|VISIBLE_IN_SILENT,"true"); printf("|\n");
}

/**
 * @brief Emit one serp() message with a controlled missing-file errno
 */
static void serp_case(void)
{
	errno = ENOENT;
	serp("Failed to open file");
}

/**
 * @brief Do nothing while function_capture() checks pending stdout behavior
 */
static void no_output_test(void)
{
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Exercise slog() when timestamp construction helpers fail
 */
static void logger_time_failure_cases(void)
{
	rational_logger_mode = VERBOSE;

	/* Missing timestamp data should leave an empty prefix slot but keep the message */
	testmocking_gettimeofday_fail_next(1);
	slog(VERBOSE,"gettimeofday failure");
	printf("\n");
	testmocking_gettimeofday_disable();

	/* localtime_r() failure follows the same fallback path */
	testmocking_localtime_r_fail_next(1);
	slog(VERBOSE,"localtime_r failure");
	printf("\n");
	testmocking_localtime_r_disable();

	/* snprintf() failure while formatting the timestamp must not hide the payload */
	testmocking_snprintf_fail_next(1);
	slog(VERBOSE,"snprintf failure");
	testmocking_snprintf_disable();

	rational_logger_mode = REGULAR;
}

/**
 * @brief Emit one report() message with a controlled I/O errno
 */
static void report_single_case(void)
{
	errno = EIO;
	report("Mocked report output");
}

/**
 * @brief Emit one report() message while snprintf is forced to truncate
 */
static void report_truncated_case(void)
{
	testmocking_snprintf_truncate_next(1);
	report_single_case();
	testmocking_snprintf_disable();
}

/**
 * @brief Capture slog() output when logger line allocation fails
 *
 * @return Return describing success or failure
 */
static Return capture_librational_logger_realloc_failure(void)
{
	INITTEST;

	/* The logger should stay silent when it cannot grow the output line */
	testmocking_realloc_fail_next(1);
	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR,"hidden");
	rational_logger_mode = REGULAR;
	testmocking_realloc_disable();

	deliver(status);
}

/**
 * @brief Capture slog() output from timestamp failure scenarios
 *
 * @return Return describing success or failure
 */
static Return capture_librational_logger_time_failures(void)
{
	INITTEST;

	/* Cleanup disables every mock in case the captured scenario exits early */
	logger_time_failure_cases();
	rational_logger_mode = REGULAR;
	testmocking_gettimeofday_disable();
	testmocking_localtime_r_disable();
	testmocking_snprintf_disable();
	testmocking_write_disable();

	deliver(status);
}

/**
 * @brief Capture slog() output when vsnprintf fails before line allocation
 *
 * @return Return describing success or failure
 */
static Return capture_librational_logger_vsnprintf_failure(void)
{
	INITTEST;

	/* A failed payload size calculation should suppress the incomplete line */
	testmocking_vsnprintf_fail_next(1);
	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR,"hidden");
	rational_logger_mode = REGULAR;
	testmocking_vsnprintf_disable();

	deliver(status);
}
#endif

/**
 * @brief Capture an ERROR log while no logger mode accepts it
 *
 * @return Return describing success or failure
 */
static Return capture_librational_logger_disabled_error_mode(void)
{
	INITTEST;

	/* With no active mode bits, even ERROR must stay silent */
	rational_logger_mode = (LOGMODES)0;
	slog(ERROR,"hidden");
	rational_logger_mode = REGULAR;

	deliver(status);
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Capture REMEMBER calls that have no usable payload
 *
 * @return Return describing success or failure
 */
static Return capture_librational_logger_remember_without_payload(void)
{
	INITTEST;

	/* Empty payload produces no remembered or printed line */
	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR|REMEMBER,"");

	/* Failed formatting also leaves REMEMBER without a payload to deliver */
	testmocking_vsnprintf_fail_next(1);
	slog(REGULAR|UNDECOR|REMEMBER,"hidden");
	testmocking_vsnprintf_disable();

	rational_logger_mode = REGULAR;

	deliver(status);
}
#endif

/**
 * @brief Capture the basic report() stderr contract
 *
 * @return Return describing success or failure
 */
static Return capture_librational_report_test(void)
{
	INITTEST;

	report_test();

	deliver(status);
}

/**
 * @brief Capture the full slog() mode matrix
 *
 * @return Return describing success or failure
 */
static Return capture_librational_slog_test(void)
{
	INITTEST;

	/* Reset the global logger mode after the matrix leaves it in SILENT */
	slog_test();
	rational_logger_mode = REGULAR;

	deliver(status);
}

/**
 * @brief Capture a log line that contains unsafe terminal bytes
 */
static void librational_logger_terminal_safety_case(void)
{
	/*
	 * The payload mixes terminal-hostile bytes with ordinary UTF-8 text.
	 * It deliberately keeps raw ESC because this first sanitizer pass preserves
	 * existing logger decorations until an explicit decoration whitelist exists
	 */
	static const char payload[] =
	        "ascii"
	        "\001"
	        "del"
	        "\177"
	        "c1"
	        "\302\220"
	        "invalid"
	        "\303("
	        "overlong"
	        "\300\257"
	        "keep"
	        "\t\r\n"
	        "esc"
	        "\033[1m"
	        "utf8"
	        "°"
	        "à"
	        "Привет"
	        "天地玄黄宇宙洪荒日月盈昃辰宿列張"
	        "いろはにほへとちりぬるを"
	        "日本語漢字仮名交じり文";

	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR,"%s",payload);
	rational_logger_mode = REGULAR;
}

/**
 * @brief Capture one serp() stderr message
 *
 * @return Return describing success or failure
 */
static Return capture_librational_serp_case(void)
{
	INITTEST;

	serp_case();

	deliver(status);
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Capture report() when formatting the final message fails
 *
 * @return Return describing success or failure
 */
static Return capture_librational_report_snprintf_failure(void)
{
	INITTEST;

	/* REPORT() should avoid writing a malformed message after snprintf failure */
	testmocking_snprintf_fail_next(1);
	report_single_case();
	testmocking_snprintf_disable();

	deliver(status);
}

/**
 * @brief Capture report() when the primary write fails but fallback write works
 *
 * @return Return describing success or failure
 */
static Return capture_librational_report_write_fallback(void)
{
	INITTEST;

	/* The first write fails, so REPORT() should emit its fixed fallback text */
	testmocking_write_fail_next(1,EIO);
	report_single_case();
	testmocking_write_disable();

	deliver(status);
}

/**
 * @brief Capture report() when both primary and fallback writes fail
 *
 * @return Return describing success or failure
 */
static Return capture_librational_report_write_full_failure(void)
{
	INITTEST;

	/* Both writes fail, so REPORT() must give up without leaking partial output */
	testmocking_write_fail_next(2,EIO);
	report_single_case();
	testmocking_write_disable();

	deliver(status);
}
#endif

/**
 * @brief Check report() formatting with errno and source location
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_1(void)
{
	INITTEST;

	/* report() output goes to stderr and includes file, function, line, message, and errno */
	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_report_stderr_pattern,
		capture_librational_report_test));

	RETURN_STATUS;
}

/**
 * @brief Check slog() mode filtering, prefixes, and silent visibility
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_2(void)
{
	INITTEST;

	/* The pattern covers every printed line; variable fields are limited to time, path, and line */
	ASSERT(SUCCESS == match_function_output(
		expected_slog_stdout_pattern,
		NULL,
		capture_librational_slog_test));

	RETURN_STATUS;
}

/**
 * @brief Check serp() formatting with errno and source location
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_3(void)
{
	INITTEST;

	/* serp() writes directly to stderr without printf-style formatting */
	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_serp_stderr_pattern,
		capture_librational_serp_case));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Check that report() stays silent when message formatting fails
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_4(void)
{
	INITTEST;

	/* NULL patterns require both captured streams to stay empty */
	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_report_snprintf_failure));

	RETURN_STATUS;
}

/**
 * @brief Check the exact buffer state after report() truncation
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_5(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	/* This test inspects raw buffer lengths, so it uses function_capture() directly */
	ASSERT(SUCCESS == function_capture(
		report_truncated_case,
		captured_stdout,
		captured_stderr));

	ASSERT(captured_stdout->length == 0U);
	ASSERT(captured_stderr->length == MAX_CHARACTERS + 1U);
	ASSERT(captured_stderr->string_length == MAX_CHARACTERS);

	const char *stderr_data = m_data_ro(char,captured_stderr);
	ASSERT(stderr_data != NULL);

	IF(stderr_data != NULL)
	{
		const char expected_tail[MAX_CHARACTERS] = {0};

		/* The snprintf mock writes one visible byte and leaves the rest zero-filled */
		ASSERT(stderr_data[0] == 'T');
		ASSERT(0 == memcmp(stderr_data + 1,expected_tail,MAX_CHARACTERS));
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check the report() fallback message after primary write failure
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_6(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_write_fallback_stderr_pattern,
		capture_librational_report_write_fallback));

	RETURN_STATUS;
}

/**
 * @brief Check that report() stays silent when both writes fail
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_report_write_full_failure));

	RETURN_STATUS;
}
#endif

/**
 * @brief Check rational_reconvert() edge cases around empty and REMEMBER modes
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_8(void)
{
	INITTEST;

	ASSERT(0 == strcmp(rational_reconvert(0),""));
	ASSERT(0 == strcmp(rational_reconvert(REMEMBER),"REMEMBER"));
	ASSERT(0 == strcmp(
		rational_reconvert(REGULAR|REMEMBER|VISIBLE_IN_SILENT),
		"REGULAR | REMEMBER | VISIBLE_IN_SILENT"));

	RETURN_STATUS;
}

/**
 * @brief Check rational_reconvert() names every logger mode flag
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_9(void)
{
	INITTEST;

	/* Pin all single-flag names and the full mapping order used in diagnostics */
	ASSERT(0 == strcmp(rational_reconvert(REGULAR),"REGULAR"));
	ASSERT(0 == strcmp(rational_reconvert(VERBOSE),"VERBOSE"));
	ASSERT(0 == strcmp(rational_reconvert(TESTING),"TESTING"));
	ASSERT(0 == strcmp(rational_reconvert(ERROR),"ERROR"));
	ASSERT(0 == strcmp(rational_reconvert(SILENT),"SILENT"));
	ASSERT(0 == strcmp(rational_reconvert(UNDECOR),"UNDECOR"));
	ASSERT(0 == strcmp(rational_reconvert(REMEMBER),"REMEMBER"));
	ASSERT(0 == strcmp(rational_reconvert(VISIBLE_IN_SILENT),"VISIBLE_IN_SILENT"));
	ASSERT(0 == strcmp(
		rational_reconvert(REGULAR|VERBOSE|TESTING|ERROR|SILENT|UNDECOR|REMEMBER|VISIBLE_IN_SILENT),
		"REGULAR | VERBOSE | TESTING | ERROR | SILENT | UNDECOR | REMEMBER | VISIBLE_IN_SILENT"));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Check that slog() stays silent when logger line allocation fails
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_10(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_realloc_failure));

	RETURN_STATUS;
}

/**
 * @brief Check slog() output when timestamp helper calls fail
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_11(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		expected_logger_time_failure_stdout_pattern,
		NULL,
		capture_librational_logger_time_failures));

	RETURN_STATUS;
}

/**
 * @brief Check that slog() stays silent when vsnprintf fails
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_12(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_vsnprintf_failure));

	RETURN_STATUS;
}
#endif

/**
 * @brief Check that slog(ERROR) stays silent when no mode accepts ERROR
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_13(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_disabled_error_mode));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Check that REMEMBER has no output when no payload is available
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_14(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_remember_without_payload));

	RETURN_STATUS;
}
#endif

/**
 * @brief Check that slog() escapes terminal-hostile text without damaging normal UTF-8
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_15(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	static const char expected_stdout[] =
	        "ascii"
	        "\\x01"
	        "del"
	        "\\x7F"
	        "c1"
	        "\\xC2\\x90"
	        "invalid"
	        "\\xC3("
	        "overlong"
	        "\\xC0\\xAF"
	        "keep"
	        "\t\r\n"
	        "esc"
	        "\033[1m"
	        "utf8"
	        "°"
	        "à"
	        "Привет"
	        "天地玄黄宇宙洪荒日月盈昃辰宿列張"
	        "いろはにほへとちりぬるを"
	        "日本語漢字仮名交じり文";

	ASSERT(SUCCESS == function_capture(
		librational_logger_terminal_safety_case,
		captured_stdout,
		captured_stderr));

	ASSERT(0 == strcmp(m_text(captured_stdout),expected_stdout));
	ASSERT(captured_stderr->length == 0U);

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Check that function_capture() flushes pending stdout before redirection
 *
 * @return Return describing success or failure
 */
static Return test_librational_0004_capture_flush(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	int saved_stdout_fd = -1;
	FILE *stdout_tmp = NULL;

	saved_stdout_fd = dup(STDOUT_FILENO);
	ASSERT(saved_stdout_fd != -1);

	/* Redirect stdout away from the terminal, then leave text pending in stdio */
	if(SUCCESS == status)
	{
		stdout_tmp = tmpfile();
	}

	ASSERT(stdout_tmp != NULL);
	ASSERT(0 == fflush(stdout));
	ASSERT(dup2(fileno(stdout_tmp),STDOUT_FILENO) != -1);
	ASSERT(fprintf(stdout,"pending stdout before function_capture") > 0);

	/* function_capture() should flush that pending text before installing its own redirection */
	ASSERT(SUCCESS == function_capture(no_output_test,captured_stdout,captured_stderr));
	ASSERT(captured_stdout->length == 0);
	ASSERT(captured_stderr->length == 0);

	IF(stdout_tmp != NULL)
	{
		(void)fflush(stdout);
	}

	if(saved_stdout_fd != -1)
	{
		if(dup2(saved_stdout_fd,STDOUT_FILENO) == -1)
		{
			testitall_failure_location_record(__FILE__,__func__,__LINE__);
			status = FAILURE;
		}

		(void)close(saved_stdout_fd);
	}

	IF(stdout_tmp != NULL)
	{
		(void)fclose(stdout_tmp);
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

/**
 * @brief Exercise report(), serp(), slog(), and logger mode conversion
 *
 * The suite checks formatted report output, plain errno reporting through serp(),
 * slog() mode filtering and decoration, logger conversion helpers, failure paths
 * for formatting/allocation/time/write helpers, and the output-capture guard that
 * flushes pending stdout before redirecting streams
 *
 * @return SUCCESS when all subtests pass
 */
Return test_librational_0004(void)
{
	INITTEST;

	TEST(test_librational_0004_capture_flush,"function_capture() flushes pending stdout before redirection");
	TEST(test_librational_0004_1,"report() writes formatted errno messages with source location");
	TEST(test_librational_0004_2,"slog() honors logger modes, decorations and silent visibility");
	TEST(test_librational_0004_3,"serp() writes errno, source file and function name");
#ifndef EVIL_EMPIRE_OS
	TEST(test_librational_0004_4,"report() stays silent when final message formatting fails");
	TEST(test_librational_0004_5,"report() keeps a truncated fixed-size message buffer terminated");
	TEST(test_librational_0004_6,"report() writes fallback text after primary write failure");
	TEST(test_librational_0004_7,"report() stays silent when primary and fallback writes fail");
#endif
	TEST(test_librational_0004_8,"rational_reconvert() covers empty and REMEMBER edge cases");
	TEST(test_librational_0004_9,"rational_reconvert() names every logger flag");
#ifndef EVIL_EMPIRE_OS
	TEST(test_librational_0004_10,"slog() stays silent when line allocation fails");
	TEST(test_librational_0004_11,"slog() keeps payloads visible when time formatting fails");
	TEST(test_librational_0004_12,"slog() stays silent when vsnprintf fails");
#endif
	TEST(test_librational_0004_13,"slog(ERROR) stays silent when no mode accepts ERROR");
	#ifndef EVIL_EMPIRE_OS
	TEST(test_librational_0004_14,"slog(REMEMBER) skips empty or unformatted payloads");
	#endif
	TEST(test_librational_0004_15,"slog() escapes unsafe terminal bytes while preserving normal UTF-8");

	RETURN_STATUS;
}
