#include "sute.h"

/**
 * @brief Initialize Config while capturing and checking its complete diagnostic output
 *
 * Direct configuration tests use TESTING=true and expect one initialization
 * message on stdout and no stderr. The initialized settings remain available
 * to the caller
 *
 * @return SUCCESS when initialization output matches the golden template and stderr is empty
 */
Return init_config_and_check_output(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	run(function_capture(init_config,captured_stdout,captured_stderr));
	run(get_file_content("templates/0045_033.txt",pattern));
	run(match_pattern(captured_stdout,pattern,"templates/0045_033.txt"));
	ASSERT(captured_stderr->length == 0U);

	call(m_del(pattern));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	provide(status);
}

/**
 * @brief Run application scenarios in order and check their exit codes and selected output streams
 *
 * The cases array must end with a zero-initialized record whose arguments is
 * NULL. An empty arguments string is a valid scenario that runs the application
 * without arguments. Execution stops at the terminator or the first failure
 *
 * Each stdout_template or stderr_template accepts a file path, the corresponding
 * STDOUT_MUST_BE_EMPTY or STDERR_MUST_BE_EMPTY macro, or NULL. A file supplies
 * the expected PCRE2 pattern. The macros supply the pattern directly and
 * require the corresponding stream to contain no bytes. NULL skips
 * comparison of that stream. Optional substitution applies to all scenarios
 * and changes each pattern in memory, leaving the template files unchanged.
 * Environment and fixture setup belong to the caller
 *
 * @param[in] cases Scenario array terminated by a record with NULL arguments
 * @param[in] template Text to replace in the patterns, or NULL to skip substitution
 * @param[in] replacement Replacement text, required when template is not NULL
 * @return SUCCESS when all scenarios match their expected exit codes and selected output streams
 */
Return run_and_match_templates(
	const struct argument_test_case *cases,
	const char                      *template,
	const char                      *replacement)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	for(size_t i = 0; cases[i].arguments != NULL && SUCCESS == status; i++)
	{
		const struct argument_test_case *test_case = &cases[i];

		m_create(char,stdout_result,MEMORY_STRING);
		m_create(char,stderr_result,MEMORY_STRING);
		m_create(char,pattern,MEMORY_STRING);

		run(runit(test_case->arguments,stdout_result,stderr_result,
			test_case->expected_return_code,STDERR_ALLOW));

		if(test_case->stdout_template != NULL)
		{
			const char *filename = test_case->stdout_template;
			if(strcmp(filename,STDOUT_MUST_BE_EMPTY) == 0)
			{
				run(m_copy_string(pattern,STDOUT_MUST_BE_EMPTY));
				filename = NULL;
			} else {
				run(get_file_content(filename,pattern));
			}

			if(template != NULL)
			{
				run(replace_placeholder(pattern,template,replacement));
			}
			run(match_pattern(stdout_result,pattern,filename));
		}

		if(test_case->stderr_template != NULL)
		{
			const char *filename = test_case->stderr_template;
			if(strcmp(filename,STDERR_MUST_BE_EMPTY) == 0)
			{
				run(m_copy_string(pattern,STDERR_MUST_BE_EMPTY));
				filename = NULL;
			} else {
				run(get_file_content(filename,pattern));
			}

			if(template != NULL)
			{
				run(replace_placeholder(pattern,template,replacement));
			}
			run(match_pattern(stderr_result,pattern,filename));
		}

		call(m_del(pattern));
		call(m_del(stderr_result));
		call(m_del(stdout_result));
	}

	provide(status);
}
