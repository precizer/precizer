#include "testitall.h"
#include <limits.h>
#include <stdint.h>
#include <wordexp.h>

#ifndef ARG_MAX
#ifdef _POSIX_ARG_MAX
#define ARG_MAX _POSIX_ARG_MAX
#else
#define ARG_MAX 4096
#endif
#endif

extern int test_main(
	int argc,
	char **argv) __attribute__((weak));

static struct
{
	int argc;
	char **argv;
	int result;
} test_main_context = {0};

static void test_main_wrapper(void)
{
	test_main_context.result = test_main(test_main_context.argc,test_main_context.argv);
}

static Return runit_external(
	const char *safe_arguments,
	const int  expected_return_code,
	bool       suppress_stderr,
	bool       suppress_stdout);

Return runit(
	const char *arguments,
	const int  expected_return_code,
	bool       suppress_stderr,
	bool       suppress_stdout,
	bool       use_external_call)
{
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	const char *safe_arguments = arguments;

	if(NULL == safe_arguments)
	{
		safe_arguments = "";
	}

	if(true == use_external_call)
	{
		status = runit_external(
			safe_arguments,
			expected_return_code,
			suppress_stderr,
			suppress_stdout);

		provide(status);
	}

	wordexp_t parsed_arguments = {0};
	bool words_allocated = false;
	size_t argc = 0U;
	char **argv = NULL;

	char *previous_cwd = NULL;
	char *saved_testing = NULL;
	bool changed_directory = false;

	if(SUCCESS == status)
	{
		const char *current_testing = getenv("TESTING");

		if(NULL != current_testing)
		{
			saved_testing = strdup(current_testing);

			if(NULL == saved_testing)
			{
				report("Memory allocation failed, requested size: %zu bytes",strlen(current_testing) + 1U);
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status)
	{
		if(0 != setenv("TESTING","true",1))
		{
			serp("Failed to set TESTING environment");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		previous_cwd = getcwd(NULL,0);

		if(NULL == previous_cwd)
		{
			serp("Failed to get current working directory");
			status = FAILURE;
		}
	}

	const char *tmpdir = getenv("TMPDIR");

	if(SUCCESS == status)
	{
		if(NULL == tmpdir)
		{
			echo(STDERR,"Environment variable TMPDIR is not set");
			status = FAILURE;

		} else {
			if(0 != chdir(tmpdir))
			{
				serp("Failed to change directory to TMPDIR");
				status = FAILURE;

			} else {
				changed_directory = true;
			}
		}
	}

	if(SUCCESS == status)
	{
		int word_status = wordexp(safe_arguments,&parsed_arguments,WRDE_NOCMD);

		if(0 != word_status)
		{
			echo(STDERR,"Failed to parse arguments \"%s\" (wordexp code %d)",safe_arguments,word_status);
			status = FAILURE;

		} else {
			words_allocated = true;
		}
	}

	if(SUCCESS == status)
	{
		argc = parsed_arguments.we_wordc + 1U;

		if(argc > (size_t)INT_MAX)
		{
			echo(STDERR,"Too many arguments for test_main: %zu",argc);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		argv = calloc(argc + 1U,sizeof(char *));

		if(NULL == argv)
		{
			report("Memory allocation failed, requested size: %zu bytes",(argc + 1U) * sizeof(char *));
			status = FAILURE;

		} else {
			argv[0] = (char *)(uintptr_t)"precizer";

			for(size_t i = 0U; i < parsed_arguments.we_wordc; i++)
			{
				argv[i + 1U] = parsed_arguments.we_wordv[i];
			}

			argv[argc] = NULL;
		}
	}

	if(SUCCESS == status)
	{
		del(STDERR);
		del(STDOUT);

		test_main_context.argc = (int)argc;
		test_main_context.argv = argv;
		test_main_context.result = 0;

		status = function_capture(test_main_wrapper,STDOUT,STDERR);

		if(SUCCESS == status)
		{
			int exit_code = test_main_context.result;

			if(STDERR->length > 0U)
			{
				if(true == suppress_stderr)
				{
					del(STDERR);

				} else {
					char *str;
					const char *stderr_view = getcstring(STDERR);
					int rt = asprintf(&str,
						YELLOW "Warning! STDERR buffer is not empty!\n"
						"Internal call:\n" YELLOW ">>" RESET "precizer %s" YELLOW "<<" RESET "\n"
						"Stderr output:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
						safe_arguments,
						stderr_view);

					if(rt > -1)
					{
						if(SUCCESS == resize(STDERR,(size_t)rt + 1U))
						{
							char *stderr_mem = data(char,STDERR);

							if(stderr_mem == NULL)
							{
								status = FAILURE;

							} else {
								memcpy(stderr_mem,str,(size_t)rt);
								stderr_mem[STDERR->length - 1U] = '\0';
							}
						}

					} else {
						report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1U);
						status = FAILURE;
					}

					free(str);

					if(SUCCESS == status)
					{
						status = FAILURE;
					}
				}
			}

			if(SUCCESS == status)
			{
				if(STDOUT->length > 0U && true == suppress_stdout)
				{
					del(STDOUT);
				}
			}

			if(SUCCESS == status)
			{
				if(expected_return_code != exit_code)
				{
					char *str;
					const char *stderr_view = getcstring(STDERR);
					const char *stdout_view = getcstring(STDOUT);
					int rt = asprintf(&str,
						YELLOW "ERROR: Unexpected exit code!" RESET "\n"
						YELLOW "Internal call:\n" YELLOW ">>" RESET "precizer %s" YELLOW "<<" RESET "\n"
						YELLOW "Exited with code " RESET "%d " YELLOW "but expected " RESET "%d\n"
						YELLOW "Stderr output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
						YELLOW "Stdout output:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
						safe_arguments,
						exit_code,
						expected_return_code,
						stderr_view,
						stdout_view);

					if(rt > -1)
					{
						if(SUCCESS == resize(STDERR,(size_t)rt + 1U))
						{
							char *stderr_mem = data(char,STDERR);

							if(stderr_mem == NULL)
							{
								status = FAILURE;

							} else {
								memcpy(stderr_mem,str,(size_t)rt);
								stderr_mem[STDERR->length - 1U] = '\0';
							}
						}

					} else {
						report("Memory allocation failed, requested size: %zu bytes",(size_t)rt + 1U);
						status = FAILURE;
					}

					free(str);

					if(SUCCESS == status)
					{
						status = FAILURE;
					}
				}
			}
		}
	}

	if(true == changed_directory && NULL != previous_cwd)
	{
		if(0 != chdir(previous_cwd))
		{
			serp("Failed to restore working directory");
			status = FAILURE;
		}
	}

	if(NULL != saved_testing)
	{
		if(0 != setenv("TESTING",saved_testing,1))
		{
			serp("Failed to restore TESTING environment");
			status = FAILURE;
		}
	}

	if(NULL == saved_testing)
	{
		(void)unsetenv("TESTING");
	}

	free(saved_testing);
	free(previous_cwd);
	free(argv);

	if(true == words_allocated)
	{
		wordfree(&parsed_arguments);
	}

	provide(status);
}

static Return runit_external(
	const char *safe_arguments,
	const int  expected_return_code,
	bool       suppress_stderr,
	bool       suppress_stdout)
{
	Return status = SUCCESS;

	const char *command_prefix = "export TESTING=true && cd ${TMPDIR} && ${BINDIR}/precizer";

	char command[ARG_MAX];
	int written = snprintf(
		command,
		sizeof(command),
		"%s %s",
		command_prefix,
		safe_arguments);

	if(written < 0 || (size_t)written >= sizeof(command))
	{
		echo(STDERR,"Command length exceeds ARG_MAX (%zu bytes)",sizeof(command));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = external_call(command,
			expected_return_code,
			suppress_stderr,
			suppress_stdout);
	}

	return(status);
}
