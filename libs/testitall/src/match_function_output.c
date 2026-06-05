#include "testitall.h"

/* function_capture() accepts a void callback and cannot return the callback
   status itself. This file-private state bridges that API to the Return-based
   contract used by match_function_output */
static Return (*function_output_target)(void) = NULL;
static Return function_output_target_status = FAILURE;

/**
 * @brief Run the Return-based function through the void-based capture API
 *
 * function_capture() accepts only a void callback, so match_function_output()
 * stores the real Return-based target in file-private state before starting
 * capture. This adapter calls that target and stores its Return value in
 * function_output_target_status so the caller can merge it with the capture
 * status after function_capture() finishes
 *
 * function_output_target is checked by match_function_output() before
 * function_capture() can call this adapter. The adapter only preserves the
 * Return value that function_capture() itself has no place to store
 */
static void run_function_output_target(void)
{
	function_output_target_status = function_output_target();
}

/**
 * @brief Match one captured stream against a PCRE2 pattern or require it to be empty
 *
 * This helper implements the stream-checking contract used by
 * match_function_output(). A non-NULL @p pattern_text is interpreted as a
 * PCRE2 regular expression and matched through @ref match_pattern. A NULL
 * @p pattern_text does not disable checking; it means the captured stream is
 * expected to be empty, equivalent to checking captured_stream->length == 0
 * in a test
 *
 * Mismatches and invalid arguments are reported through @ref echo in the same
 * STDERR diagnostic channel used by the other match helpers
 *
 * @param[in] captured_stream Captured stdout or stderr descriptor
 * @param[in] pattern_text PCRE2 pattern for the stream, or NULL to require silence
 * @param[in] stream_name Human-readable stream name used in diagnostics
 * @return SUCCESS when the stream matches the expectation, FAILURE otherwise
 */
static Return match_captured_stream(
	const memory *captured_stream,
	const char   *pattern_text,
	const char   *stream_name)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(captured_stream == NULL || stream_name == NULL)
	{
		echo(STDERR,"ERROR: Invalid arguments for captured stream matching\n");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		/* A NULL pattern means "expect no output", not "skip this stream".
		   This keeps silent-stream checks in the same helper as regex checks */
		if(pattern_text == NULL)
		{
			if(captured_stream->length != 0)
			{
				const char *captured_text = m_text(captured_stream);

				if(captured_text == NULL)
				{
					echo(STDERR,"ERROR: Captured %s was expected to be empty, but its text view is unavailable\n",stream_name);
				} else {
					echo(STDERR,
						YELLOW "ERROR: Captured %s was expected to be empty, but contains:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
						stream_name,
						captured_text);
				}

				status = FAILURE;
			}
		} else {
			/* The pattern descriptor is needed only for the regex branch.
			   Empty-stream checks above avoid creating even this stack descriptor */
			m_create(char,pattern,MEMORY_STRING);

			status = m_copy_string(pattern,pattern_text);

			if((TRIUMPH & status) == 0)
			{
				echo(STDERR,"ERROR: Failed to prepare captured %s pattern for matching\n",stream_name);
			}

			if(TRIUMPH & status)
			{
				run(match_pattern(captured_stream,pattern,stream_name));
			}

			call(m_del(pattern));
		}
	}

	provide(status);
}

/**
 * @brief Capture a function's stdout and stderr and match both streams
 *
 * Runs @p func through @ref function_capture, then checks both captured
 * streams. A non-NULL pattern argument is treated as a PCRE2 regular
 * expression for the corresponding stream. A NULL pattern argument requires
 * that stream to be empty, which replaces the common
 * captured_stdout->length == 0 and captured_stderr->length == 0 boilerplate
 * used by tests
 *
 * The captured function must follow the testitall Return contract. Its status
 * is preserved by the local adapter around function_capture() and merged with
 * the status of the capture operation itself. Stream matching runs only after
 * both the capture mechanism and the captured function finish successfully
 *
 * @param[in] stdout_pattern PCRE2 pattern for captured stdout, or NULL to require empty stdout
 * @param[in] stderr_pattern PCRE2 pattern for captured stderr, or NULL to require empty stderr
 * @param[in] func Function whose output streams should be captured
 * @return SUCCESS when capture succeeds and both streams match, FAILURE otherwise
 */
Return match_function_output(
	const char *stdout_pattern,
	const char *stderr_pattern,
	Return     (*func)(void))
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	if(func == NULL)
	{
		echo(STDERR,"ERROR: Function output matching requires a non-NULL function pointer\n");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		/* function_capture() captures stdout and stderr around a void callback.
		   The actual Return-producing test function is called through the
		   adapter above, and its status is read back from file-private state */
		function_output_target = func;
		function_output_target_status = SUCCESS;

		Return capture_status = function_capture(run_function_output_target,captured_stdout,captured_stderr);
		Return function_status = function_output_target_status;

		/* Clear the adapter state after the capture so a later call starts from
		   an explicit empty state even if this call failed */
		function_output_target = NULL;
		function_output_target_status = FAILURE;

		if((TRIUMPH & capture_status) == 0)
		{
			echo(STDERR,"ERROR: function_capture failed while matching function output: %s\n",show_status(capture_status));
		}

		/* The final status must inherit both possible failure sources: the
		   capture mechanism itself and the function that was captured */
		status = rational_normalize_return(status | capture_status | function_status);
	}

	if(TRIUMPH & status)
	{
		run(match_captured_stream(captured_stdout,stdout_pattern,"stdout"));
		run(match_captured_stream(captured_stderr,stderr_pattern,"stderr"));
	}

	call(m_del(captured_stdout));
	call(m_del(captured_stderr));

	provide(status);
}
