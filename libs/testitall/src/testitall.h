#ifndef TESTITALL_H
#define TESTITALL_H

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

#ifdef APP_NAME
#define TESTITALL_APP_NAME APP_NAME
#else
#ifndef TESTITALL_APP_NAME
#define TESTITALL_APP_NAME "testitall"
#endif
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
				testitall_failure_location_record(__FILE__,__func__,__LINE__); \
				status = FAILURE; \
			} \
		}

/**
 * @def IF(condition)
 * @brief Guard code that is safe to run only after a matching assertion passed
 * @param condition Boolean expression that protects the guarded block
 *
 * Use this macro only for defensive branches in tests, typically after
 * ASSERT(condition), or for cleanup branches in test infrastructure where
 * the branch only checks whether a temporary resource was actually opened.
 * Coverage builds exclude this macro from branch accounting, so regular
 * test logic must still use plain `if`
 */
	#define IF(condition) \
		if(condition)

	#define RETURN_STATUS \
		/* Count this completed test function for the final runner summary */ \
		testitall_completed_tests++; \
		if(SUCCESS == status) \
		{ \
			echo(EXTEND,BOLDGREEN "✓" BOLDWHITE " passed" RESET); \
		} else { \
			if(testitall_failure_location_report(EXTEND) == false) \
			{ \
				echo(EXTEND,BOLDRED "𐄂" BOLDWHITE " failed" RESET); \
			} \
		} \
		deliver(status); \

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
typedef enum CAPTURE_POLICY : unsigned int
{
	STDOUT_ENABLE = 1u << 0,
	STDOUT_SUPPRESS = 1u << 1,
	STDERR_ENABLE = 1u << 2,
	STDERR_SUPPRESS = 1u << 3,
	STDERR_ALLOW = 1u << 4,
	ALLOW_BOTH = STDOUT_ENABLE | STDERR_ENABLE,   // Hex: 0x05. Dec: 5. Bin: 00101
	SUPPRESS_BOTH_BUFFERS = STDOUT_SUPPRESS | STDERR_SUPPRESS      // Hex: 0x0A. Dec: 10. Bin: 01010
} CAPTURE_POLICY;

// Global buffers for capturing output streams.
extern memory _STDOUT;
extern memory *STDOUT;
extern memory _STDERR;
extern memory *STDERR;
extern memory _EXTEND;
extern memory *EXTEND;

extern size_t testitall_completed_tests;

void testitall_failure_location_record(
	const char *,
	const char *,
	int);

void testitall_failure_location_clear(void);

bool testitall_failure_location_report(memory *);

Return external_call(
	const char *,
	memory *,
	memory *,
	const int,
	CAPTURE_POLICY);

void echo(
	memory *,
	const char *,
	...) __attribute__((format(printf,2,3)));

Return execute_command(
	const char *,
	memory *,
	memory *,
	const int,
	CAPTURE_POLICY);

Return execute_and_set_variable(
	const char *,
	const char *,
	const int);

Return set_environment_variable(
	const char *,
	const char *);

/**
 * @brief Write the parent directory of the current working directory to a buffer
 *
 * @param[out] path Destination memory descriptor initialized for char elements
 * @return SUCCESS on success or FAILURE on error
 */
Return determine_origin_dir(memory *);

/**
 * @brief Create a unique temporary directory and keep its path in the buffer
 *
 * @param[out] path Destination memory descriptor initialized for char elements
 * @return SUCCESS on success or FAILURE on error
 */
Return create_tmpdir(memory *);

Return function_capture(
	void (*)(void),
	memory *,
	memory *);

Return get_file_content(
	const char *,
	memory *);

Return compare_memory_strings(
	memory *,
	const memory *,
	const memory *);

Return match_function_output(
	const char *,
	const char *,
	Return (*)(void));

Return match_pattern(
	const memory *,
	const memory *,
	...);

/**
 * @brief Execute a shell command and match its output against a file template
 *
 * @param[in] command Command string to execute
 * @param[in] filename Template file path
 * @param[in] template Placeholder token to replace in the template
 * @param[in] replacement Replacement text for the placeholder token
 * @param[in] expected_return_code Expected process exit status
 * @return SUCCESS when template preparation, command execution, and matching succeed
 */
Return match_file_template(
	const char *,
	const char *,
	const char *,
	const char *,
	const int);

/**
 * @brief Run the tested application and match its output against a file template
 *
 * @param[in] arguments Command-line arguments passed without the binary name
 * @param[in] filename Template file path
 * @param[in] template Placeholder token to replace in the template
 * @param[in] replacement Replacement text for the placeholder token
 * @param[in] expected_return_code Expected process exit status
 * @return SUCCESS when template preparation, application run, and matching succeed
 */
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

/**
 * @brief Construct a path by joining `$TMPDIR` and a child path
 *
 * @param[in] filename Child path relative to `$TMPDIR`
 * @param[out] full_path Destination memory descriptor initialized for char elements
 * @return SUCCESS on success or FAILURE on error
 */
Return construct_path(
	const char *,
	memory *);

/**
 * @brief Generate a random integer in the inclusive `[start..end]` range
 *
 * @param[out] random_number Output random value
 * @param[in] start Inclusive lower bound
 * @param[in] end Inclusive upper bound
 * @return SUCCESS on success or FAILURE on error
 */
Return random_number_generator_urandom(
	uint64_t *,
	uint64_t,
	uint64_t
);

/**
 * @brief Generate a pseudorandom integer in the inclusive `[start..end]` range
 *
 * @param[out] random_number Output random value
 * @param[in] start Inclusive lower bound
 * @param[in] end Inclusive upper bound
 * @return SUCCESS on success or FAILURE on error
 */
Return random_number_generator_xoshiro(
	uint64_t *,
	uint64_t,
	uint64_t
);

/**
 * @brief Extract the name of the directory that contains the current executable
 *
 * Writes the final path segment of the current executable directory into
 * @p environment, for example `debug` from
 * `/worktree/.builds/testitall/debug/testitall`
 *
 * @param environment Output memory descriptor initialized for char elements
 * @return SUCCESS on success, FAILURE on error
 */
Return extract_current_executable_directory_name(memory *);

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
	testitall_failure_location_clear(); \
	/* The status that will be returned before exiting. */ \
	/* By default, assumes the function ran without errors. */ \
	Return status = SUCCESS; \
	bool first_header = true;

// Macro to measure the end time of a test.
#define SUTEDONE \
	long long int _test_end_time = cur_time_ns(); \
	long long int _time_spent = _test_end_time - _test_start_time; \
	printf(WHITE "Total execution time: %s\n" RESET,form_date(_time_spent,FULL_VIEW)); \
	printf(WHITE "Total completed tests: %zu\n" RESET,testitall_completed_tests); \
	if(TRIUMPH & status) \
	{ \
		printf(WHITE "Completed " BOLDGREEN "successfully\n" RESET); \
		return((int)COMPLETED); \
	} else { \
		printf(WHITE "Ended " BOLDRED "unsuccessfully\n" RESET); \
		printf("Exit status » %s\n",show_status(status)); \
		return((int)status); \
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
		deliver(DONOTHING); \
	}

Return testitall(
	Return (*)(void),
	const char *,
	const char *);

enum run_mode
{
	INTERNAL_TEST = 0,
	EXTERNAL_CALL = 1
};

extern enum run_mode run_external;

extern int test_main(
	int,
	char **) __attribute__((weak));

Return runit(
	const char *,
	memory *,
	memory *,
	const int,
	CAPTURE_POLICY);

/**
 * @brief Run precizer in background and complete signal-driven scenario.
 *
 * The helper starts the process, configures delay control environment variables
 * (`TESTITALL_SIGNAL_WAIT_MS`, `TESTITALL_SIGNAL_WAIT_POINT`, and optionally
 * `TESTITALL_SIGNAL_WAIT_READY_FD`), sends `signal_number` to the child, waits
 * for completion, and finalizes output capture and exit-code validation.
 *
 * When `min_delay_ms` is greater than zero, the helper waits that long before
 * signal delivery. When `min_delay_ms` is zero, the helper waits until the
 * child reports that the configured wait point was reached, then sends the
 * signal immediately.
 *
 * If the child exits before signal delivery, the helper returns failure.
 *
 * `max_delay_ms` is used as:
 * - value for `TESTITALL_SIGNAL_WAIT_MS`
 * - watchdog timeout (SIGKILL after timeout if child is still alive)
 * - wait-point notification timeout when `min_delay_ms` is zero
 *
 * @note Supports both EXTERNAL_CALL and INTERNAL_TEST modes.
 */
Return runit_background(
	const char *,
	memory *,
	memory *,
	const int,
	CAPTURE_POLICY,
	uint64_t,
	uint64_t,
	int,
	unsigned int);

Return trim_trailing_eol(memory *);

extern bool show_subheader;

#endif /* TESTITALL_H */
