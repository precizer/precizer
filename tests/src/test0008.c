#include "sute.h"

static void report_test(void){
	// Report an error with formatted message
	report("Memory reallocation failed with bytes %d",10);
	// Report an error with multiple arguments
	report("Buffer overflow at position %d with value %s",42,"overflow");
}

Return test0008(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	MSTRUCT(mem_char,captured_stdout);
	MSTRUCT(mem_char,captured_stderr);
	char *pattern = NULL;

	ASSERT(SUCCESS == function_capture(report_test,captured_stdout,captured_stderr));

	#if 0
	printf("captured_stderr:%s",captured_stderr->mem);
	printf("captured_stdout:%s\n",captured_stdout->mem);
	#endif

	if(captured_stdout->length > 0)
	{
		echo(STDERR,"ERROR: Stdout buffer is not empty. It contains characters: %zu\n",captured_stdout->length);
		status = FAILURE;
	}

	ASSERT(SUCCESS == get_file_content("templates/0008.txt",&pattern));

	ASSERT(SUCCESS == match_pattern(captured_stderr->mem,pattern));

	reset(&pattern);

	del_char(&captured_stdout);
	del_char(&captured_stderr);

	RETURN_STATUS;
}
