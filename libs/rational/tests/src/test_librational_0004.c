#include "sute.h"
#include "testmocking.h"
#include <errno.h>

static const char expected_report_stderr_pattern[] =
	"\\A"
	"ERROR: [^\\n]*src/test_librational_0004\\.c:report_test:\\d+ Memory reallocation failed with bytes 10 Errno: [^\\n]+ \\(errno: [0-9]+\\)\n"
	"ERROR: [^\\n]*src/test_librational_0004\\.c:report_test:\\d+ Buffer overflow at position 42 with value overflow Errno: [^\\n]+ \\(errno: [0-9]+\\)\\Z";

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
	".*"
	"Mode: SILENT\n"
	"42\\. Must print in SILENT without prefixes:\\|true\\|\n"
	"43\\. Must print no ERROR prefix in SILENT:\\|true\\|\\Z";

static const char expected_serp_stderr_pattern[] =
	"\\A"
	"ERROR: Failed to open file \\[File: [^\\n]*src/test_librational_0004\\.c, Function: serp_case\\] Errno: \\(2\\) No such file or directory\\Z";

static const char expected_write_fallback_stderr_pattern[] =
	"\\AERROR: Failed to write error message\n\\Z";

static const char expected_logger_time_failure_stdout_pattern[] =
	"\\A"
	" [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:gettimeofday failure\n"
	" [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:localtime_r failure\n"
	" [^\\n]*src/test_librational_0004\\.c:[0-9]+:logger_time_failure_cases:snprintf failure\\Z";

static void report_test(void)
{
	errno = ENOMEM;
	// Report an error with formatted message
	report("Memory reallocation failed with bytes %d",10);
	errno = EOVERFLOW;
	// Report an error with multiple arguments
	report("Buffer overflow at position %d with value %s",42,"overflow");
}

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

static void serp_case(void)
{
	errno = ENOENT;
	serp("Failed to open file");
}

static void no_output_test(void)
{
}

static void logger_time_failure_cases(void)
{
	rational_logger_mode = VERBOSE;

	testmocking_gettimeofday_fail_next(1);
	slog(VERBOSE,"gettimeofday failure");
	printf("\n");
	testmocking_gettimeofday_disable();

	testmocking_localtime_r_fail_next(1);
	slog(VERBOSE,"localtime_r failure");
	printf("\n");
	testmocking_localtime_r_disable();

	testmocking_snprintf_fail_next(1);
	slog(VERBOSE,"snprintf failure");
	testmocking_snprintf_disable();

	rational_logger_mode = REGULAR;
}

static void report_single_case(void)
{
	errno = EIO;
	report("Mocked report output");
}

static void report_truncated_case(void)
{
	testmocking_snprintf_truncate_next(1);
	report_single_case();
	testmocking_snprintf_disable();
}

static Return capture_librational_logger_realloc_failure(void)
{
	INITTEST;

	testmocking_realloc_fail_next(1);
	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR,"hidden");
	rational_logger_mode = REGULAR;
	testmocking_realloc_disable();

	deliver(status);
}

static Return capture_librational_logger_time_failures(void)
{
	INITTEST;

	logger_time_failure_cases();
	rational_logger_mode = REGULAR;
	testmocking_gettimeofday_disable();
	testmocking_localtime_r_disable();
	testmocking_snprintf_disable();
	testmocking_write_disable();

	deliver(status);
}

static Return capture_librational_logger_vsnprintf_failure(void)
{
	INITTEST;

	testmocking_vsnprintf_fail_next(1);
	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR,"hidden");
	rational_logger_mode = REGULAR;
	testmocking_vsnprintf_disable();

	deliver(status);
}

static Return capture_librational_logger_disabled_error_mode(void)
{
	INITTEST;

	rational_logger_mode = (LOGMODES)0;
	slog(ERROR,"hidden");
	rational_logger_mode = REGULAR;

	deliver(status);
}

static Return capture_librational_logger_remember_without_payload(void)
{
	INITTEST;

	rational_logger_mode = REGULAR;
	slog(REGULAR|UNDECOR|REMEMBER,"");

	testmocking_vsnprintf_fail_next(1);
	slog(REGULAR|UNDECOR|REMEMBER,"hidden");
	testmocking_vsnprintf_disable();

	rational_logger_mode = REGULAR;

	deliver(status);
}

static Return capture_librational_report_test(void)
{
	INITTEST;

	report_test();

	deliver(status);
}

static Return capture_librational_slog_test(void)
{
	INITTEST;

	slog_test();
	rational_logger_mode = REGULAR;

	deliver(status);
}

static Return capture_librational_serp_case(void)
{
	INITTEST;

	serp_case();

	deliver(status);
}

static Return capture_librational_report_snprintf_failure(void)
{
	INITTEST;

	testmocking_snprintf_fail_next(1);
	report_single_case();
	testmocking_snprintf_disable();

	deliver(status);
}

static Return capture_librational_report_write_fallback(void)
{
	INITTEST;

	testmocking_write_fail_next(1,EIO);
	report_single_case();
	testmocking_write_disable();

	deliver(status);
}

static Return capture_librational_report_write_full_failure(void)
{
	INITTEST;

	testmocking_write_fail_next(2,EIO);
	report_single_case();
	testmocking_write_disable();

	deliver(status);
}

static Return test_librational_0004_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_report_stderr_pattern,
		capture_librational_report_test));

	RETURN_STATUS;
}

static Return test_librational_0004_2(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		expected_slog_stdout_pattern,
		NULL,
		capture_librational_slog_test));

	RETURN_STATUS;
}

static Return test_librational_0004_3(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_serp_stderr_pattern,
		capture_librational_serp_case));

	RETURN_STATUS;
}

static Return test_librational_0004_4(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_report_snprintf_failure));

	RETURN_STATUS;
}

static Return test_librational_0004_5(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

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

		ASSERT(stderr_data[0] == 'T');
		ASSERT(0 == memcmp(stderr_data + 1,expected_tail,MAX_CHARACTERS));
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	RETURN_STATUS;
}

static Return test_librational_0004_6(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_write_fallback_stderr_pattern,
		capture_librational_report_write_fallback));

	RETURN_STATUS;
}

static Return test_librational_0004_7(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_report_write_full_failure));

	RETURN_STATUS;
}

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

static Return test_librational_0004_10(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_realloc_failure));

	RETURN_STATUS;
}

static Return test_librational_0004_11(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		expected_logger_time_failure_stdout_pattern,
		NULL,
		capture_librational_logger_time_failures));

	RETURN_STATUS;
}

static Return test_librational_0004_12(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_vsnprintf_failure));

	RETURN_STATUS;
}

static Return test_librational_0004_13(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_disabled_error_mode));

	RETURN_STATUS;
}

static Return test_librational_0004_14(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	ASSERT(SUCCESS == match_function_output(
		NULL,
		NULL,
		capture_librational_logger_remember_without_payload));

	RETURN_STATUS;
}

static Return test_librational_0004_capture_flush(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	int saved_stdout_fd = -1;
	FILE *stdout_tmp = NULL;

	saved_stdout_fd = dup(STDOUT_FILENO);
	ASSERT(saved_stdout_fd != -1);

	if(SUCCESS == status)
	{
		stdout_tmp = tmpfile();
	}

	ASSERT(stdout_tmp != NULL);
	ASSERT(0 == fflush(stdout));
	ASSERT(dup2(fileno(stdout_tmp),STDOUT_FILENO) != -1);
	ASSERT(fprintf(stdout,"pending stdout before function_capture") > 0);
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
 * @brief librational report and logger tests
 *
 * @return SUCCESS when all subtests pass
 */
Return test_librational_0004(void)
{
	INITTEST;

	TEST(test_librational_0004_capture_flush,"function_capture flushes pending stdout before redirection…");
	TEST(test_librational_0004_1,"librational test report messaging…");
	TEST(test_librational_0004_2,"librational test slog messaging…");
	TEST(test_librational_0004_3,"librational test serp messaging…");
	TEST(test_librational_0004_4,"librational report handles formatting failure without output…");
	TEST(test_librational_0004_5,"librational report writes a truncated fixed-size message…");
	TEST(test_librational_0004_6,"librational report writes fallback text after write failure…");
	TEST(test_librational_0004_7,"librational report stays silent when primary and fallback writes fail…");
	TEST(test_librational_0004_8,"librational mode reconversion covers empty and REMEMBER modes…");
	TEST(test_librational_0004_10,"librational logger stays silent when line allocation fails…");
	TEST(test_librational_0004_11,"librational logger handles time formatting failures…");
	TEST(test_librational_0004_12,"librational logger stays silent when vsnprintf fails…");
	TEST(test_librational_0004_13,"librational logger keeps ERROR silent when no mode accepts it…");
	TEST(test_librational_0004_14,"librational logger skips REMEMBER without payload…");

	RETURN_STATUS;
}
