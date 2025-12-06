#include "sute.h"

static void report_test(void)
{
	// Report an error with formatted message
	report("Memory reallocation failed with bytes %d",10);
	// Report an error with multiple arguments
	report("Buffer overflow at position %d with value %s",42,"overflow");
}

/*
 * Testing the formatted error message output function.
 * This function's key feature is that it displays messages
 * without heap memory allocation. For example, it can be
 * used to output messages about memory allocation failures
 * when there is insufficient memory available.
 */
Return test0008(void)
{
	INITTEST;

	create(char,captured_stdout);
	create(char,captured_stderr);
	create(char,pattern);

	ASSERT(SUCCESS == function_capture(report_test,captured_stdout,captured_stderr));

	#if 0
	printf("captured_stderr:%s",getcstring(captured_stderr));
	printf("captured_stdout:%s\n",getcstring(captured_stdout));
	#endif

	if(captured_stdout->length > 0)
	{
		echo(STDERR,"ERROR: Stdout buffer is not empty. It contains characters: %zu\n",captured_stdout->length);
		status = FAILURE;
	}

	ASSERT(SUCCESS == get_file_content("templates/0008.txt",pattern));

	ASSERT(SUCCESS == match_pattern(captured_stderr,pattern));

	del(pattern);

	del(captured_stdout);
	del(captured_stderr);

	RETURN_STATUS;
}
