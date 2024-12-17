#pragma once

// Need for strdup(), clock_gettime()
// Have to be at the beginning of the file
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
#include "pcre2.h"

#include <stdbool.h>
// Functions to work with time
#include <time.h>
#include <sys/time.h>

// Work with strings
#include <string.h>

// librational
#include "rational.h"

// libmem
#include "mem.h"

// Default buffer memory size
#define BUFFER_LENGTH 1024

/**
 * @brief Prints a formatted header message if status check passes
 * @details Outputs the given message in cyan color if the status equals SUCCESS
 *
 * @param msg The message to be printed (should include newline if needed)
 *
 * @note Requires a 'status' variable to be in scope
 * @note Assumes SUCCESS is defined elsewhere in the codebase
 *
 * @example
 * // Example usage:
 * HEADER("Preparations\n");
 */
#define HEADER(msg) \
	if(SUCCESS == status) \
	{ \
		printf(CYAN msg RESET); \
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
		echo(EXTEND, BOLDGREEN "✓" BOLDWHITE " passed " RESET); \
	} else { \
		echo(EXTEND, BOLDRED "𐄂" BOLDWHITE " failed" RESET); \
	} \
	return(status); \


// Global buffers for capturing output streams
extern mem_char _STDOUT;
extern mem_char *STDOUT;
extern mem_char _STDERR;
extern mem_char *STDERR;
extern mem_char _EXTEND;
extern mem_char *EXTEND;

// Macros to define default value of third аrgument
// https://stackoverflow.com/questions/1472138/c-default-arguments
// https://gustedt.wordpress.com/2010/06/03/default-arguments-for-c99/
//#define external_call(...) EXTERNAL_CALL(__VA_ARGS__, false, 0)
//#define EXTERNAL_CALL(a, b, c, ...) external_call(a, b, c)
#define external_call(...) EXTERNAL_CALL(__VA_ARGS__, false, 0, false, 0)
#define EXTERNAL_CALL(a, b, c, d, e, ...) external_call(a, b, c, d)

#define execute_command(...) EXECUTE_COMMAND(__VA_ARGS__, false, 0, false, 0)
#define EXECUTE_COMMAND(a, b, c, d, e, f, ...) execute_command(a, b, c, d, e)

Return external_call(
	const char *,
	const int,
	bool,
	bool
);

void echo(
	mem_char *,
	const char *,
	...
);

Return execute_command(
	const char *,
	mem_char *,
	const int,
	bool,
	bool
);

Return execute_and_set_variable(
	const char *,
	const char *,
	const int
);

Return set_environment_variable(
	const char *,
	const char *
);

Return function_capture(
	void (*func)(void),
	mem_char *,
	mem_char *
);

Return get_file_content(
	const char *,
	char **
);

Return match_pattern(
	const char *,
	const char *
);

Return match_file_template(
	const char *,
	const char *,
	const char *,
	const char *,
	const int
);

Return replace_placeholder(
	char **,
	const char *,
	const char *
);

Return write_to_temp_file(
	const char *
);

Return check_file_exists(
	bool *,
	const char *
);

/**
 * @brief Test execution macro
 * @param func Function to test
 * @param desc Test description
 */
#define TEST(func, desc) \
	if (SUCCESS == status) \
	{ \
		status = testitall(func, #func, desc); \
	}

#define EXEC(func, desc) \
	status = testitall(func, #func, desc);

// Execute a function without checking the status first.
// For example, to clear temporary data
#define TESTFINISH(func, desc) \
	(void)testitall(func, #func, desc);

// Макрос для замера времени на старте
#define TESTSTART \
	long long int _test_start_time = cur_time_ns(); \
	/* The status that will be passed to return() before exiting */ \
	/* By default, the function worked without errors.           */ \
	Return status = SUCCESS;

// Макрос для замера времени на финише
#define TESTDONE \
	long long int _test_end_time = cur_time_ns(); \
	long long int _time_spent = _test_end_time - _test_start_time; \
	printf(RESET WHITE "Total execution time: %lldns (%s)\n", _time_spent, form_date(_time_spent)); \
	if(SUCCESS == status) \
	{ \
		printf(RESET WHITE "Completed " BOLDGREEN "successfully\n"); \
	} else { \
		printf(RESET WHITE "Ended " BOLDRED "unsuccessfully\n"); \
	} \
	return(status);

Return testitall(
	Return (*func)(void),
	const char *,
	const char *
);
