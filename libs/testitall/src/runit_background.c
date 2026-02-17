#include "testitall.h"

#include "runit_internal.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>

/**
 * @brief Snapshot of wait-control environment variables.
 *
 * Stores previous values of TESTITALL_SIGNAL_WAIT_MS and
 * TESTITALL_SIGNAL_WAIT_POINT so they can be restored after the run.
 */
enum runit_wait_env_key
{
	RUNIT_WAIT_ENV_MS = 0,
	RUNIT_WAIT_ENV_POINT,
	RUNIT_WAIT_ENV_COUNT
};

static const char *const runit_wait_env_names[RUNIT_WAIT_ENV_COUNT] = {
	"TESTITALL_SIGNAL_WAIT_MS",
	"TESTITALL_SIGNAL_WAIT_POINT"
};

struct runit_wait_env
{
	char *old_value[RUNIT_WAIT_ENV_COUNT];
	bool had_value[RUNIT_WAIT_ENV_COUNT];
	bool snapshot_ready;
};

/**
 * @brief Reset wait-environment snapshot state.
 */
static void runit_wait_env_init(struct runit_wait_env *state)
{
	memset(state,0,sizeof(*state));
}

/**
 * @brief Free memory held by wait-environment snapshot state.
 */
static void runit_wait_env_cleanup(struct runit_wait_env *state)
{
	for(size_t i = 0U; i < RUNIT_WAIT_ENV_COUNT; i++)
	{
		free(state->old_value[i]);
	}
	runit_wait_env_init(state);
}

static uint64_t runit_wait_env_target_value(
	const enum runit_wait_env_key key,
	uint64_t                      max_delay_ms,
	unsigned int                  wait_point)
{
	if(RUNIT_WAIT_ENV_MS == key)
	{
		return(max_delay_ms);
	}

	return((uint64_t)wait_point);
}

/**
 * @brief Sleep for the requested time in milliseconds.
 *
 * The sleep is split into bounded chunks so EINTR handling stays simple and
 * long waits do not depend on one oversized timespec value.
 */
static void sleep_for_milliseconds(uint64_t timeout_ms)
{
	while(timeout_ms > 0U)
	{
		uint64_t chunk_ms = timeout_ms;
		if(chunk_ms > 1000U)
		{
			chunk_ms = 1000U;
		}

		struct timespec request = {
			.tv_sec = (time_t)(chunk_ms / 1000U),
			.tv_nsec = (long)((chunk_ms % 1000U) * 1000000ULL)
		};

		while(nanosleep(&request,&request) == -1 && errno == EINTR)
		{
		}

		timeout_ms -= chunk_ms;
	}
}

/**
 * @brief Kill parent process with SIGKILL when timeout expires.
 *
 * This helper runs in a watchdog child process. It exits early if the parent
 * PID changes, which means the target process already exited.
 */
static void run_watchdog(
	const pid_t  target_pid,
	uint64_t     timeout_ms,
	const size_t interval_ms)
{
	while(timeout_ms > 0U)
	{
		if(getppid() != target_pid)
		{
			_exit((int)COMPLETED);
		}

		uint64_t chunk_ms = timeout_ms;
		if(chunk_ms > interval_ms)
		{
			chunk_ms = interval_ms;
		}

		sleep_for_milliseconds(chunk_ms);
		timeout_ms -= chunk_ms;
	}

	if(getppid() == target_pid)
	{
		(void)kill(target_pid,SIGKILL);
	}

	_exit((int)COMPLETED);
}

/**
 * @brief Snapshot one environment variable for later restore.
 */
static Return runit_wait_env_snapshot(
	const char *name,
	char      **saved_value,
	bool       *had_value)
{
	Return status = SUCCESS;

	*saved_value = NULL;
	*had_value = false;

	const char *current_value = getenv(name);
	if(NULL != current_value)
	{
		*had_value = true;
		*saved_value = strdup(current_value);
		if(NULL == *saved_value)
		{
			report("Memory allocation failed while saving %s",name);
			status = FAILURE;
		}
	}

	provide(status);
}

/**
 * @brief Set one environment variable from an unsigned numeric value.
 */
static Return runit_wait_env_set_numeric(
	const char *name,
	uint64_t    value)
{
	Return status = SUCCESS;
	char value_text[32];

	(void)snprintf(value_text,sizeof(value_text),"%" PRIu64,value);

	if(0 != setenv(name,value_text,1))
	{
		echo(STDERR,"Failed to set %s (errno %d)",name,errno);
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Restore one saved environment variable value.
 */
static Return runit_wait_env_restore_one(
	const char *name,
	const char *saved_value,
	bool        had_value)
{
	Return status = SUCCESS;

	if(true == had_value)
	{
		if(0 != setenv(name,saved_value,1))
		{
			echo(STDERR,"Failed to restore %s (errno %d)",name,errno);
			status = FAILURE;
		}

	} else {
		if(0 != unsetenv(name))
		{
			echo(STDERR,"Failed to unset %s (errno %d)",name,errno);
			status = FAILURE;
		}
	}

	provide(status);
}

/**
 * @brief Set wait-control environment variables for the target application.
 *
 * The current values are copied to caller-owned storage so they can be
 * restored after the background run finishes.
 */
static Return runit_wait_env_set(
	uint64_t      max_delay_ms,
	unsigned int  wait_point,
	struct runit_wait_env *state)
{
	Return status = SUCCESS;

	for(size_t i = 0U; i < RUNIT_WAIT_ENV_COUNT; i++)
	{
		run(runit_wait_env_snapshot(
			runit_wait_env_names[i],
			&state->old_value[i],
			&state->had_value[i]));
	}

	if(SUCCESS == status)
	{
		state->snapshot_ready = true;
	}

	for(size_t i = 0U; i < RUNIT_WAIT_ENV_COUNT; i++)
	{
		const uint64_t target_value = runit_wait_env_target_value(
			(enum runit_wait_env_key)i,
			max_delay_ms,
			wait_point);

		run(runit_wait_env_set_numeric(runit_wait_env_names[i],target_value));
	}

	provide(status);
}

/**
 * @brief Restore previously saved wait-control environment variable values.
 */
static Return runit_wait_env_restore(
	const struct runit_wait_env *state)
{
	Return status = SUCCESS;

	for(size_t i = 0U; i < RUNIT_WAIT_ENV_COUNT; i++)
	{
		call(runit_wait_env_restore_one(
			runit_wait_env_names[i],
			state->old_value[i],
			state->had_value[i]));
	}

	provide(status);
}

/**
 * @brief Run precizer in background and complete signal-driven scenario.
 *
 * The function starts a child process, configures wait-control environment
 * variables, waits for @p min_delay_ms, attempts to send @p signal_number to
 * the child, and then waits for completion while finalizing stdout/stderr
 * capture.
 *
 * If the child exits before signal delivery, the function reports failure.
 *
 * Internal helper-to-helper calls inside this closed API use lightweight
 * argument checks by design to improve readability and reduce overhead.
 * Keep this approach in future runit* internal helpers as well: avoid redundant
 * argument validation in internal-only functions that are not externally visible
 * and exist to remove code duplication.
 */
Return runit_background(
	const char   *arguments,
	memory       *stdout_result,
	memory       *stderr_result,
	const int    expected_return_code,
	unsigned int buffer_policy,
	uint64_t     min_delay_ms,
	uint64_t     max_delay_ms,
	int          signal_number,
	unsigned int wait_point)
{
	Return status = SUCCESS;
	const char *safe_arguments = "";
	if(arguments != NULL && '\0' != arguments[0])
	{
		safe_arguments = arguments;
	}

	struct runit_call runit_call_data;
	runit_call_init(&runit_call_data);

	struct runit_capture_session capture;
	runit_capture_session_init(&capture);

	pid_t app_pid = (pid_t)-1;
	bool child_started = false;
	int wait_status = 0;
	bool child_waited = false;

	struct runit_wait_env wait_environment;
	runit_wait_env_init(&wait_environment);

	call(del(STDOUT));
	call(del(STDERR));

	run(runit_validate_runtime_mode(run_external));

	if(SUCCESS == status && 0U == max_delay_ms)
	{
		echo(STDERR,"Background max delay must be greater than 0 milliseconds");
		status = FAILURE;
	}

	if(SUCCESS == status && min_delay_ms > max_delay_ms)
	{
		echo(STDERR,
			"Background min delay must not exceed max delay (%" PRIu64 " > %" PRIu64 ")",
			min_delay_ms,
			max_delay_ms);
		status = FAILURE;
	}

	if(SUCCESS == status && signal_number <= 0)
	{
		echo(STDERR,"Signal number must be positive, got: %d",signal_number);
		status = FAILURE;
	}

	run(runit_prepare_call_and_capture(
		&runit_call_data,
		&capture,
		run_external,
		safe_arguments,
		"Background call",
		stdout_result,
		stderr_result,
		expected_return_code,
		buffer_policy));

	/* max_delay_ms also defines how long in-app wait helpers may pause. */
	run(runit_wait_env_set(
		max_delay_ms,
		wait_point,
		&wait_environment));

	if(SUCCESS == status)
	{
		app_pid = fork();
		if(app_pid < 0)
		{
			serp("Failed to fork background process");
			status = FAILURE;

		} else if(0 == app_pid) {
			if(chdir(runit_call_data.tmpdir) != 0)
			{
				_exit(127);
			}

			if(SUCCESS != runit_capture_apply_redirect(&capture))
			{
				_exit(127);
			}

			/* Separate watchdog process enforces hard timeout with SIGKILL. */
			const pid_t protected_pid = getpid();
			const pid_t watchdog_pid = fork();
			if(watchdog_pid < 0)
			{
				_exit(127);
			}

			if(watchdog_pid == 0)
			{
				run_watchdog(protected_pid,max_delay_ms,25U);
			}

			if(EXTERNAL_CALL == run_external)
			{
				(void)execv(runit_call_data.program_path,runit_call_data.argv);
				_exit(127);
			}

			const int test_main_result = test_main((int)runit_call_data.argc,runit_call_data.argv);
			fflush(stdout);
			fflush(stderr);
			_exit(test_main_result);

		} else {
			child_started = true;
			/* Parent keeps paths only; capture fds are no longer needed in this process. */
			runit_capture_close_fds(&capture);
		}
	}

	if(SUCCESS == status)
	{
		/* Give the app time to reach signal_wait_at_point(), then inject the requested signal. */
		sleep_for_milliseconds(min_delay_ms);

		pid_t poll_pid = (pid_t)-1;
		do {
			poll_pid = waitpid(app_pid,&wait_status,WNOHANG);
		} while(poll_pid == (pid_t)-1 && errno == EINTR);

		if(poll_pid == app_pid)
		{
			child_waited = true;
			echo(STDERR,"Background process exited before signal delivery");
			status = FAILURE;

		} else if(poll_pid == 0) {
			errno = 0;
			if(0 != kill(app_pid,signal_number))
			{
				echo(STDERR,
					"Failed to send signal %d to background PID %d (errno %d)",
					signal_number,
					(int)app_pid,
					errno);
				status = FAILURE;
			}

		} else {
			echo(STDERR,
				"Failed to poll background PID %d before signal (errno %d)",
				(int)app_pid,
				errno);
			status = FAILURE;
		}
	}

	if(true == child_started)
	{
		Return wait_return_status = SUCCESS;

		if(false == child_waited)
		{
			wait_return_status = runit_wait_child(app_pid,&wait_status,"background process");
			if(SUCCESS == wait_return_status)
			{
				child_waited = true;
			}
		}

		if(SUCCESS == wait_return_status && true == child_waited)
		{
			wait_return_status = runit_capture_finalize(&capture,wait_status);
		}

		if(SUCCESS == status && SUCCESS != wait_return_status)
		{
			status = wait_return_status;
		}
	}

	if(true == wait_environment.snapshot_ready)
	{
		call(runit_wait_env_restore(&wait_environment));
	}

	runit_wait_env_cleanup(&wait_environment);
	runit_release_call_and_capture(&capture,&runit_call_data);

	run(del(STDERR));

	provide(status);
}
