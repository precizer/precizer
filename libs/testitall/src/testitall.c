#include "testitall.h"

bool show_subheader = false;

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
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Start time measurement */
	long long int __start_time = cur_time_ns();

	/* Clear output capture buffers to ensure clean state */
	call(del(STDOUT));
	call(del(STDERR));

	if(show_subheader == true)
	{
		printf(BLUE " » %s" RESET "\n",test_description);
		show_subheader = false;
	}

	/* Execute the test function and capture its return status */
	status = func();

	/* Calculate execution time */
	long long int __end_time = cur_time_ns();
	long long int elapsed_time = __end_time - __start_time;
	const char *elapsed_time_text = form_date(elapsed_time,MAJOR_VIEW);

	/* Format and display test results with color coding */
	if(SUCCESS == status)
	{
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[  " BOLDGREEN "OK" RESET WHITE  "  ]" RESET );
		fprintf(stdout,WHITE " %s %s %s" RESET,function_name,elapsed_time_text,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = getcstring(EXTEND);

		if((EXTEND->length > 0) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");

	} else if(DONOTHING & status){
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[ " BOLDYELLOW "SKIP" RESET WHITE  " ]" RESET );
		fprintf(stdout,WHITE " %s %s %s" RESET,function_name,elapsed_time_text,test_description);

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
		fprintf(stdout,WHITE " %s %s %s" RESET,function_name,elapsed_time_text,test_description);

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
		fprintf(stdout,RED "STDERR" RESET " " WHITE "is not empty when it should be:\n" RESET);
		fprintf(stdout,"%s",stderr_buffer);
	}

	/* Display captured stdout output */
	const char *stdout_buffer = getcstring(STDOUT);

	if((STDOUT->length > 0) && (stdout_buffer[0] != '\0'))
	{
		fprintf(stdout,BLUE "STDOUT" RESET " " WHITE "is not empty when it should be:\n" RESET);
		fprintf(stdout,"%s",stdout_buffer);
	}

	/* Cleanup: free dynamically allocated buffers */
	call(del(STDOUT));
	call(del(STDERR));
	call(del(EXTEND));

	deliver(status);
}
