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
	int  argc,
	char **argv) __attribute__((weak));

enum run_mode run_external = EXTERNAL_CALL;

static struct {
	int argc;
	char **argv;
	int result;
} test_main_context = {0};

static void test_main_wrapper(void)
{
	test_main_context.result = test_main(test_main_context.argc,test_main_context.argv);
}

Return runit(
	const char *arguments,
	memory     *result,
	const int  expected_return_code,
	bool       suppress_stderr,
	bool       suppress_stdout)
{
	// Base status for the whole sequence; subsequent steps update it on errors.
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	// Arguments string to parse and forward into test_main
	const char *safe_arguments = arguments;

	if(NULL == safe_arguments || safe_arguments[0] == '\0')
	{
		safe_arguments = "";
	}

	// External mode: build shell command and exit early.
	if(EXTERNAL_CALL == run_external)
	{
		char command[ARG_MAX];
		int written = snprintf(
			command,
			sizeof(command),
			"cd ${TMPDIR} && ${BINDIR}/precizer %s",
			safe_arguments);

		if(written < 0 || (size_t)written >= sizeof(command))
		{
			echo(STDERR,"Command length exceeds ARG_MAX (%zu bytes)",sizeof(command));
			status = FAILURE;

		} else {
			run(execute_command(command,result,expected_return_code,suppress_stderr,suppress_stdout));
		}

		provide(status);
	}

	// Prepare wordexp and argv/argc for test_main.
	wordexp_t parsed_arguments = {0};
	bool words_allocated = false;
	size_t argc = 0U;
	char **argv = NULL;

	// Save current working directory to restore after test_main.
	char *previous_cwd = NULL;
	bool changed_directory = false;

	// Capture working directory before switching to TMPDIR.
	if(SUCCESS == status)
	{
		previous_cwd = getcwd(NULL,0);

		if(NULL == previous_cwd)
		{
			serp("Failed to get current working directory");
			status = FAILURE;
		}
	}

	// Switch into TMPDIR (tests expect to run precizer from a temp directory).
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

	// Parse argument string into words without command substitution.
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

	// Ensure argument count fits into int and set argc.
	if(SUCCESS == status)
	{
		argc = parsed_arguments.we_wordc + 1U;

		if(argc > (size_t)INT_MAX)
		{
			echo(STDERR,"Too many arguments for test_main: %zu",argc);
			status = FAILURE;
		}
	}

	// Build argv: argv[0] is a program name, the rest are parsed words.
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

	// Run test_main in-process while capturing stdout/stderr.
	if(SUCCESS == status)
	{
		call(del(STDERR));
		call(del(STDOUT));

		test_main_context.argc = (int)argc;
		test_main_context.argv = argv;
		test_main_context.result = 0;

		run(function_capture(test_main_wrapper,STDOUT,STDERR));

		if(SUCCESS == status)
		{
			int exit_code = test_main_context.result;

			// Handle stderr: either suppress it or format a warning and fail.
			if(STDERR->length > 0U)
			{
				if(true == suppress_stderr)
				{
					call(del(STDERR));

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
							run(copy_literal(STDERR,str));
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

			// Suppress stdout if requested.
			if(STDOUT->length > 0U && true == suppress_stdout)
			{
				call(del(STDOUT));
			}

			// Compare exit code with expected and format a report on mismatch.
			if(SUCCESS == status)
			{
				if(expected_return_code != exit_code)
				{
					char *str;
					const char *stderr_view = getcstring(STDERR);
					const char *stdout_view = getcstring(STDOUT);
					int rt = asprintf(&str,
						YELLOW "ERROR: Unexpected exit code!" RESET "\n"
						YELLOW "Internal call:" RESET "\n" YELLOW ">>" RESET "precizer %s" YELLOW "<<" RESET "\n"
						YELLOW "Exited with code " RESET "%d" YELLOW " but expected " RESET "%d\n"
						YELLOW "Stderr output:" RESET "\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n"
						YELLOW "Stdout output:" RESET "\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
						safe_arguments,
						exit_code,
						expected_return_code,
						stderr_view,
						stdout_view);

					if(rt > -1)
					{
						if(SUCCESS == resize(STDERR,(size_t)rt + 1U))
						{
							run(copy_literal(STDERR,str));
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

	// Restore original working directory if changed.
	if(true == changed_directory && NULL != previous_cwd)
	{
		if(0 != chdir(previous_cwd))
		{
			serp("Failed to restore working directory");
			status = FAILURE;
		}
	}

	free(previous_cwd);
	free(argv);

	// Copy stdout into caller-provided result buffer when requested.
	if(true == words_allocated)
	{
		wordfree(&parsed_arguments);
	}

	if(NULL != result)
	{
		if(STDOUT->length > 0U)
		{
			run(copy(result,STDOUT));
		}
	}

	call(del(STDOUT));

	provide(status);
}
