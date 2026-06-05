#pragma once

#include "testitall.h"

#include <sys/wait.h>
#include <wordexp.h>

/**
 * @brief Captured stdout/stderr files and validation context for one runit call.
 */
struct runit_capture_session {
	char stdout_path[PATH_MAX];
	char stderr_path[PATH_MAX];
	int stdout_fd;
	int stderr_fd;
	const char *arguments;
	const char *call_label;
	memory *stdout_result;
	memory *stderr_result;
	int expected_return_code;
	CAPTURE_POLICY buffer_policy;
};

void runit_capture_session_init(struct runit_capture_session *);

void runit_capture_close_fds(struct runit_capture_session *);

Return runit_capture_prepare(
	struct runit_capture_session *,
	const char *,
	const char *,
	const char *,
	memory *,
	memory *,
	int,
	CAPTURE_POLICY);

Return runit_capture_apply_redirect(struct runit_capture_session *);

Return runit_capture_finalize(
	struct runit_capture_session *,
	int);

void runit_capture_cleanup(struct runit_capture_session *);

Return runit_reset_result_buffers(
	memory *,
	memory *);

Return runit_validate_runtime_mode(enum run_mode);

/**
 * @brief Prepared execution context shared by runit() and runit_background().
 *
 * The structure owns temporary resources used to build and execute the target
 * command (argument expansion, argv storage, and executable path storage).
 */
struct runit_call {
	const char *safe_arguments;
	const char *tmpdir;
	char *program_path;
	char **argv;
	size_t argc;
	wordexp_t parsed_arguments;
	bool words_allocated;
	memory program_path_buffer;
	memory argv_buffer;
};

/**
 * @brief Initialize a runit_call structure to a clean state.
 */
void runit_call_init(struct runit_call *);

/**
 * @brief Release resources owned by runit_call and reset it.
 */
void runit_call_cleanup(struct runit_call *);

/**
 * @brief Build executable path and argv for EXTERNAL_CALL or INTERNAL_TEST mode.
 *
 * Validates required environment variables, expands arguments using wordexp(),
 * and fills the provided runit_call structure.
 */
Return runit_call_prepare(
	struct runit_call *,
	enum run_mode,
	const char *);

/**
 * @brief Prepare call arguments and capture session for one runit-style execution.
 *
 * Also clears caller-provided output buffers (`stdout_result` and
 * `stderr_result`) before preparing capture files and metadata.
 */
Return runit_prepare_call_and_capture(
	struct runit_call *,
	struct runit_capture_session *,
	enum run_mode,
	const char *,
	const char *,
	memory *,
	memory *,
	int,
	CAPTURE_POLICY);

/**
 * @brief Cleanup helper for runit call/capture pair.
 */
void runit_release_call_and_capture(
	struct runit_capture_session *,
	struct runit_call *);

/**
 * @brief Wait for a specific child process and handle EINTR retries.
 */
Return runit_wait_child(
	pid_t,
	int *,
	const char *);
