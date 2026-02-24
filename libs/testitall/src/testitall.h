#pragma once

// This definition is used to disable specific functions in the application being tested,
// such as the main() function.
#ifndef TESTITALL
#define TESTITALL
#endif

#define _GNU_SOURCE
#ifdef EVIL_EMPIRE_OS
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
/*
 * Evil Empire OS uses `st_*timespec` fields instead of `st_*tim`.
 * Map the member names so we can keep the Linux-oriented copy code.
 */
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#define st_atim st_atimespec
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>

// Functions for string manipulation.
#include <string.h>
#include <strings.h>

// librational library.
#include "rational.h"

// libmem library.
#include "mem.h"

// libxdiff library.
#include "xdiff.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/**
 * @brief Prints a formatted header message if the status check passes.
 * @details Outputs the provided message in cyan if the status equals SUCCESS.
 *
 * @param msg The message to print (include a newline if needed).
 *
 * @note Requires a 'status' variable to be in scope.
 * @note Assumes SUCCESS is defined elsewhere in the codebase.
 *
 * @example
 * // Example usage:
 * HEADER("Preparations");
 */
#define HEADER(msg) \
	if(SUCCESS == status) \
	{ \
		if(first_header == true) \
		{ \
			first_header = false; \
		} else { \
			printf("\n"); \
		} \
		printf(CYAN msg RESET "\n"); \
	}

#define ASSERT(condition) \
	if(SUCCESS == status) \
	{ \
		if(condition) \
		{ \
			status = SUCCESS; \
		} else { \
			status = FAILURE; \
		} \
	}

#define RETURN_STATUS \
	if(SUCCESS == status) \
	{ \
		echo(EXTEND,BOLDGREEN "✓" BOLDWHITE " passed " RESET); \
	} else { \
		echo(EXTEND,BOLDRED "𐄂" BOLDWHITE " failed" RESET); \
	} \
	deliver(status); \

// Global buffers for capturing output streams.
extern memory _STDOUT;
extern memory *STDOUT;
extern memory _STDERR;
extern memory *STDERR;
extern memory _EXTEND;
extern memory *EXTEND;

Return external_call(
	const char *,
	memory *,
	memory *,
	const int,
	unsigned int);

void echo(
	memory *,
	const char *,
	...) __attribute__((format(printf,2,3)));

Return execute_command(
	const char *,
	memory *,
	memory *,
	const int,
	unsigned int);

Return execute_and_set_variable(
	const char *,
	const char *,
	const int);

Return set_environment_variable(
	const char *,
	const char *);

Return get_origin_dir(
	char *,
	size_t);

Return create_tmpdir(
	char *,
	size_t);

Return function_capture(
	void  (*func)(void),
	memory *,
	memory *);

Return get_file_content(
	const char *,
	memory *);

Return match_pattern(
	const memory *,
	const memory *,
	...);

Return match_file_template(
	const char *,
	const char *,
	const char *,
	const char *,
	const int);

Return match_app_output(
	const char *,
	const char *,
	const char *,
	const char *,
	const int);

Return replace_placeholder(
	memory *,
	const char *,
	const char *);

Return write_to_temp_file(const char *);

Return check_file_exists(
	bool *,
	const char *);

Return get_file_stat(
	const char *,
	struct stat *);

Return check_file_identity(
	const struct stat *,
	const struct stat *);

Return construct_path(
	const char *,
	memory *);

Return random_number_generator(
	uint64_t *,
	uint64_t,
	uint64_t
);

Return extract_current_executable_directory_name(
	char *,
	size_t);

#define SKIPPED 100500

/**
 * @brief Macro for executing a test.
 * @param func The function to test.
 * @param desc Description of the test.
 */
#define TEST(func,desc) \
	if(SUCCESS == status) \
	{ \
		status = testitall(func, #func,desc); \
	}

#define SUTE(func,desc) \
	if(SUCCESS == status) \
	{ \
		show_subheader = true; \
		status = testitall(func, #func,desc); \
	}

#define EXEC(func,desc) \
	status = testitall(func, #func,desc);

// Executes a function without checking the status first.
// Useful for tasks like clearing temporary data.
// Does not return a Result.
#define RUN(func,desc) \
	(void)testitall(func, #func,desc);

// Macro to measure the start time of a test.
#define SUTESTART \
	long long int _test_start_time = cur_time_ns(); \
	/* The status that will be returned before exiting. */ \
	/* By default, assumes the function ran without errors. */ \
	Return status = SUCCESS; \
	bool first_header = true;

// Macro to measure the end time of a test.
#define SUTEDONE \
	long long int _test_end_time = cur_time_ns(); \
	long long int _time_spent = _test_end_time - _test_start_time; \
	printf(WHITE "Total execution time: %s\n" RESET,form_date(_time_spent,FULL_VIEW)); \
	if(TRIUMPH & status) \
	{ \
		printf(WHITE "Completed " BOLDGREEN "successfully\n" RESET); \
		deliver(COMPLETED); \
	} else { \
		printf(WHITE "Ended " BOLDRED "unsuccessfully\n" RESET); \
		deliver(status); \
	}

// Initializes a test. Defines the return value as SUCCESS or FAILURE.
#define INITTEST \
	/* The status that will be returned before exiting. */ \
	/* By default, assumes the function ran without errors. */ \
	Return status = SUCCESS;

#define SLOWTEST \
	const char *compare_string = "skip"; \
	const char *env_var = getenv("SLOWTEST"); \
	/* Check if it exists and compare it to "skip" */ \
	if(env_var != NULL && strncasecmp(env_var,compare_string,strlen(compare_string)) == 0) \
	{ \
		echo(EXTEND,BOLDYELLOW "↯" BOLDWHITE " skipped" RESET); \
		deliver(SKIPPED); \
	}

Return testitall(
	Return (*func)(void),
	const char *,
	const char *);

enum run_mode
{
	INTERNAL_TEST = 0,
	EXTERNAL_CALL = 1
};

/**
 * @brief Bitmask flags for stdout/stderr capture.
 *
 * Flags are ORed together:
 * - STDOUT_SUPPRESS: drop captured stdout.
 * - STDERR_SUPPRESS: drop captured stderr and do not fail on it.
 * - STDERR_ALLOW: keep stderr without failing; wins over STDERR_SUPPRESS.
 * - STDOUT_ENABLE/STDERR_ENABLE and ALLOW_BOTH are informational only.
 * - SUPPRESS_BOTH_BUFFERS is STDOUT_SUPPRESS|STDERR_SUPPRESS.
 */
enum capture_policy
{
	STDOUT_ENABLE         = 1U << 0,
	STDOUT_SUPPRESS       = 1U << 1,
	STDERR_ENABLE         = 1U << 2,
	STDERR_SUPPRESS       = 1U << 3,
	STDERR_ALLOW          = 1U << 4,
	ALLOW_BOTH            = STDOUT_ENABLE|STDERR_ENABLE,
	SUPPRESS_BOTH_BUFFERS = STDOUT_SUPPRESS|STDERR_SUPPRESS
};

extern enum run_mode run_external;

extern int test_main(
	int  argc,
	char **argv) __attribute__((weak));

Return runit(
	const char *,
	memory *,
	memory *,
	const int,
	unsigned int);

/**
 * @brief Run precizer in background and complete signal-driven scenario.
 *
 * The helper starts the process, configures delay control environment variables
 * (`TESTITALL_SIGNAL_WAIT_MS` and `TESTITALL_SIGNAL_WAIT_POINT`), waits
 * `min_delay_ms`, attempts to send `signal_number` to the child, waits for
 * completion, and finalizes output capture and exit-code validation.
 *
 * If the child exits before signal delivery, the helper returns failure.
 *
 * `max_delay_ms` is used both as:
 * - value for `TESTITALL_SIGNAL_WAIT_MS`
 * - watchdog timeout (SIGKILL after timeout if child is still alive)
 *
 * @note Supports both EXTERNAL_CALL and INTERNAL_TEST modes.
 */
Return runit_background(
	const char   *,
	memory       *,
	memory       *,
	const int,
	unsigned int,
	uint64_t,
	uint64_t,
	int,
	unsigned int);

Return trim_trailing_eol(memory *);

extern bool show_subheader;
