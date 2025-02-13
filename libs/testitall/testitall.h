#pragma once

// This definition is used to disable specific functions in the application being tested,
// such as the main() function.
#ifndef TESTITALL
#define TESTITALL
#endif

// Required for strdup(), clock_gettime().
// Must be placed at the beginning of the file.
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdarg.h>
#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <stdbool.h>

// Functions for working with time.
#include <time.h>
#include <sys/time.h>

// Functions for string manipulation.
#include <string.h>

// librational library.
#include "rational.h"

// libmem library.
#include "mem.h"

// libxdiff library.
#include "xdiff.h"

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
 * HEADER("Preparations\n");
 */
#define HEADER(msg) \
	if(SUCCESS == status) \
	{ \
		printf("\n" CYAN msg RESET "\n"); \
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
	return(status); \

// Global buffers for capturing output streams.
extern mem_char _STDOUT;
extern mem_char *STDOUT;
extern mem_char _STDERR;
extern mem_char *STDERR;
extern mem_char _EXTEND;
extern mem_char *EXTEND;

Return external_call(
	const char *,
	const int,
	bool,
	bool);

void echo(
	mem_char *,
	const char *,
	...) __attribute__((format(gnu_printf,2,3)));

Return execute_command(
	const char *,
	mem_char *,
	const int,
	bool,
	bool);

Return execute_and_set_variable(
	const char *,
	const char *,
	const int);

Return set_environment_variable(
	const char *,
	const char *);

Return function_capture(
	void (*func)(void),
	mem_char *,
	mem_char *);

Return get_file_content(
	const char *,
	char **);

Return match_pattern(
	const char *,
	const char *,
	...);

Return match_file_template(
	const char *,
	const char *,
	const char *,
	const char *,
	const int);

Return replace_placeholder(
	char **,
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
	char **);

Return random_number_generator(
	uint64_t *,
	uint64_t,
	uint64_t
);

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

#define EXEC(func,desc) \
	status = testitall(func, #func,desc);

// Executes a function without checking the status first.
// Useful for tasks like clearing temporary data.
// Does not return a Result.
#define RUN(func,desc) \
	(void)testitall(func, #func,desc);

// Macro to measure the start time of a test.
#define TESTSTART \
	long long int _test_start_time = cur_time_ns(); \
	/* The status that will be returned before exiting. */ \
	/* By default, assumes the function ran without errors. */ \
	Return status = SUCCESS;

// Macro to measure the end time of a test.
#define TESTDONE \
	long long int _test_end_time = cur_time_ns(); \
	long long int _time_spent = _test_end_time - _test_start_time; \
	printf(WHITE "Total execution time: %lldns (%s)\n" RESET,_time_spent,form_date(_time_spent)); \
	if(SUCCESS == status) \
	{ \
		printf(WHITE "Completed " BOLDGREEN "successfully\n" RESET); \
	} else { \
		printf(WHITE "Ended " BOLDRED "unsuccessfully\n" RESET); \
	} \
	return(status);

// Initializes a test. Defines the return value as SUCCESS or FAILURE.
#define INITTEST \
	/* The status that will be returned before exiting. */ \
	/* By default, assumes the function ran without errors. */ \
	Return status = SUCCESS;

Return testitall(
	Return (*func)(void),
	const char *,
	const char *);
