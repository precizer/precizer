#include "testitall.h"

bool show_subheader = false;

static testitall_test_main_callback registered_test_main = NULL;

/**
 * @brief Number of test functions that reached their normal result reporting step
 *
 * The runner uses this value to show a simple final summary for people
 * reading the test output. Each completed test function adds one mark here
 * before it returns its status to the surrounding suite
 */
size_t testitall_completed_tests = 0;

/**
 * @brief Register the application entry point used by INTERNAL_TEST mode
 *
 * @param callback Function that accepts argc and argv and returns an application exit code
 */
void testitall_set_test_main(testitall_test_main_callback callback)
{
	registered_test_main = callback;
}

/**
 * @brief Return the application entry point registered for INTERNAL_TEST mode
 *
 * @return Registered callback, or NULL when INTERNAL_TEST mode is unavailable
 */
testitall_test_main_callback testitall_get_test_main(void)
{
	return registered_test_main;
}

static struct {
	bool has_failure;
	const char *file;
	const char *function;
	int line;
} failure_location = { false,NULL,NULL,0 };

/**
 * @brief Record the first assertion location for the current failing test process
 *
 * The record is written only when there is no pending failure location.
 * This keeps the original assertion location intact while the failure status
 * travels through helper functions and suite wrappers
 *
 * @param file Source file where the assertion failed
 * @param function Function where the assertion failed
 * @param line Source line where the assertion failed
 */
void testitall_failure_location_record(
	const char *file,
	const char *function,
	int        line)
{
	if(failure_location.has_failure == false)
	{
		if(file == NULL)
		{
			file = "unknown file";
		}

		if(function == NULL)
		{
			function = "unknown function";
		}

		failure_location.file = file;
		failure_location.function = function;
		failure_location.line = line;
		failure_location.has_failure = true;
	}
}

/**
 * @brief Clear the pending failure location for the current test process
 */
void testitall_failure_location_clear(void)
{
	failure_location.has_failure = false;
	failure_location.file = NULL;
	failure_location.function = NULL;
	failure_location.line = 0;
}

/**
 * @brief Append the pending failure-location report to a test output buffer
 *
 * @param output Buffer that receives the formatted failure details
 * @return true if a pending failure location was reported, false otherwise
 */
bool testitall_failure_location_report(memory *output)
{
	bool reported = false;

	if(output != NULL && failure_location.has_failure == true)
	{
		echo(output,BOLDRED "𐄂" BOLDWHITE " failed at %s:%d in %s()" RESET,failure_location.file,failure_location.line,failure_location.function);
		testitall_failure_location_clear();
		reported = true;
	}

	return reported;
}

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
	Return (   *func )(void),
	const char *function_name,
	const char *test_description)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Start time measurement */
	long long int __start_time = cur_time_ns();

	/* Clear output capture buffers to ensure clean state */
	call(m_del(STDOUT));
	call(m_del(STDERR));

	if(show_subheader == true)
	{
		printf(BLUE " » %s" RESET "\n",test_description);
		show_subheader = false;
	}

	/* Execute the test function and capture its return status */
	status = func();

	if(SUCCESS != status)
	{
		(void)testitall_failure_location_report(EXTEND);
	}

	/* Calculate execution time */
	long long int __end_time = cur_time_ns();
	long long int elapsed_time = __end_time - __start_time;
	const char *elapsed_time_text = form_date(elapsed_time,MAJOR_VIEW);

	/* Format and display test results with color coding */
	if(SUCCESS == status)
	{
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[  " BOLDGREEN "OK" RESET WHITE  "  ]" RESET );
		fprintf(stdout,WHITE " %s %s %s…" RESET,function_name,elapsed_time_text,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = m_text(EXTEND);

		if((EXTEND->string_length > 0U) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");

	} else if(DONOTHING & status){
		/* Green OK for passed tests */
		fprintf(stdout,WHITE "[ " BOLDYELLOW "SKIP" RESET WHITE  " ]" RESET );
		fprintf(stdout,WHITE " %s %s %s…" RESET,function_name,elapsed_time_text,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = m_text(EXTEND);

		if((EXTEND->string_length > 0U) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");
		status = SUCCESS;
	} else {
		/* Red FAIL for failed tests */
		fprintf(stdout,WHITE "[ " BOLDRED    "FAIL" RESET WHITE " ]" RESET );
		fprintf(stdout,WHITE " %s %s %s…" RESET,function_name,elapsed_time_text,test_description);

		/* Display any additional info captured in EXTEND buffer */
		const char *extend_buffer = m_text(EXTEND);

		if((EXTEND->string_length > 0U) && (extend_buffer[0] != '\0'))
		{
			fprintf(stdout,WHITE " %s" RESET,extend_buffer);
		}
		fprintf(stdout,"\n");
	}

	/* Display captured stderr output in yellow */
	const char *stderr_buffer = m_text(STDERR);

	if((STDERR->string_length > 0U) && (stderr_buffer[0] != '\0'))
	{
		fprintf(stdout,RED "STDERR" RESET " " WHITE "is not empty when it should be:\n" RESET);
		fprintf(stdout,"%s",stderr_buffer);
	}

	/* Display captured stdout output */
	const char *stdout_buffer = m_text(STDOUT);

	if((STDOUT->string_length > 0U) && (stdout_buffer[0] != '\0'))
	{
		fprintf(stdout,BLUE "STDOUT" RESET " " WHITE "is not empty when it should be:\n" RESET);
		fprintf(stdout,"%s",stdout_buffer);
	}

	/* Cleanup: free dynamically allocated buffers */
	call(m_del(STDOUT));
	call(m_del(STDERR));
	call(m_del(EXTEND));

	deliver(status);
}
