#include "testitall.h"

/**
 * @brief Main testing framework function that executes and evaluates test cases
 *
 * @param func Pointer to the test function to be executed
 * @param function_name String containing the name of the function being tested
 * @param test_description String containing description of the test
 *
 * @return Return Status code indicating test results:
 *         - SUCCESS: Test passed successfully
 *         - FAILURE: Test failed
 *
 * @details This function provides a framework for executing test cases and capturing their output.
 *          It measures execution time, captures stdout/stderr output, and formats the results
 *          in a readable way with color coding.
 */
Return testitall(
	Return    (*func)(void),
	const char *function_name,
	const char *test_description)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Start time measurement */
	long long int __start_time = cur_time_ns();

	/* Clear output capture buffers to ensure clean state */
	del(STDOUT);
	del(STDERR);

	/* Execute the test function and capture its return status */
	status = func();

	/* Calculate execution time */
	long long int __end_time = cur_time_ns();
	long long int elapsed_time = __end_time - __start_time;

	/* Format and display test results with color coding */
	if(SUCCESS == status)
	{
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[  " BOLDGREEN "OK" RESET WHITE  "  ]" RESET );
		fprintf(stdout,WHITE " %lldns %s %s" RESET,elapsed_time,function_name,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = getcstring(EXTEND);

		if((EXTEND->length > 0) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");

	} else if(SKIPPED == status){
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[ " BOLDYELLOW "SKIP" RESET WHITE  " ]" RESET );
		fprintf(stdout,WHITE " %lldns %s %s" RESET,elapsed_time,function_name,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = getcstring(EXTEND);

		if((EXTEND->length > 0) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");
		status = SUCCESS;
	} else {
		/* Red FAIL for failed tests */
		fprintf(stdout,WHITE "[ " BOLDRED    "FAIL" RESET WHITE " ]" RESET );
		fprintf(stdout,WHITE " %lldns %s %s" RESET,elapsed_time,function_name,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = getcstring(EXTEND);

		if((EXTEND->length > 0) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");
	}

	/* Display captured stderr output in yellow */
	const char *stderr_buffer = getcstring(STDERR);

	if((STDERR->length > 0) && (stderr_buffer[0] != '\0'))
	{
		fprintf(stdout,YELLOW "%s" RESET,stderr_buffer);
	}

	/* Display captured stdout output */
	const char *stdout_buffer = getcstring(STDOUT);

	if((STDOUT->length > 0) && (stdout_buffer[0] != '\0'))
	{
		fprintf(stdout,"%s",stdout_buffer);
	}

	/* Cleanup: free dynamically allocated buffers */
	del(STDOUT);
	del(STDERR);
	del(EXTEND);

	return(status);
}
