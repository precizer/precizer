#include "sute.h"

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
	 * The output between the '|' markers should contain only the message payload.
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
}
/**
 * All available combinations of slog options
 *
 */
Return test0009(void)
{
	INITTEST;

	create(char,captured_stdout);
	create(char,captured_stderr);

	create(char,pattern);

	ASSERT(SUCCESS == function_capture(slog_test,captured_stdout,captured_stderr));

	#if 0
	printf("captured_stderr:%s",getcstring(captured_stderr));
	printf("captured_stdout:%s",getcstring(captured_stdout));
	#endif

	ASSERT(captured_stderr->length == 0);

	ASSERT(SUCCESS == get_file_content("templates/0009.txt",pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern));

	del(pattern);

	del(captured_stdout);
	del(captured_stderr);

	RETURN_STATUS;
}
