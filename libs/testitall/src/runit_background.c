#include "testitall.h"

#include "runit_internal.h"

#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

/**
 * @brief Snapshot of wait-control environment variables.
 *
 * Stores previous values of TESTITALL_SIGNAL_WAIT_MS and
 * TESTITALL_SIGNAL_WAIT_POINT, plus the optional synchronization pipe
 * descriptor, so they can be restored after the run.
 */
enum runit_wait_env_key
{
	RUNIT_WAIT_ENV_MS = 0,
	RUNIT_WAIT_ENV_POINT,
	RUNIT_WAIT_ENV_READY_FD,
	RUNIT_WAIT_ENV_COUNT
};

static const char *const runit_wait_env_names[RUNIT_WAIT_ENV_COUNT] = {
	"TESTITALL_SIGNAL_WAIT_MS",
	"TESTITALL_SIGNAL_WAIT_POINT",
	"TESTITALL_SIGNAL_WAIT_READY_FD"
};

enum
{
	RUNIT_SIGNAL_SYNC_DISABLED_FD = -1
};

struct runit_signal_sync {
	int read_fd;
	int write_fd;
	bool enabled;
};

struct runit_wait_env {
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
	unsigned int                  wait_point,
	int                           ready_fd)
{
	if(RUNIT_WAIT_ENV_MS == key)
	{
		return(max_delay_ms);
	}

	if(RUNIT_WAIT_ENV_POINT == key)
	{
		return((uint64_t)wait_point);
	}

	return((uint64_t)ready_fd);
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
		{}

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
	char       **saved_value,
	bool       *had_value)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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

	deliver(status);
}

/**
 * @brief Set one environment variable from an unsigned numeric value.
 */
static Return runit_wait_env_set_numeric(
	const char *name,
	uint64_t   value)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char value_text[32];

	(void)snprintf(value_text,sizeof(value_text),"%" PRIu64,value);

	if(0 != setenv(name,value_text,1))
	{
		echo(STDERR,"Failed to set %s (errno %d)",name,errno);
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Unset one wait-control environment variable
 *
 * Used to prevent stale externally supplied variables from affecting a
 * background run when that specific wait feature is disabled
 *
 * @param[in] name Environment variable name to unset
 * @return SUCCESS when the variable was unset, otherwise FAILURE
 */
static Return runit_wait_env_unset_one(const char *name)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(0 != unsetenv(name))
	{
		echo(STDERR,"Failed to unset %s (errno %d)",name,errno);
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Restore one saved environment variable value.
 */
static Return runit_wait_env_restore_one(
	const char *name,
	const char *saved_value,
	bool       had_value)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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

	deliver(status);
}

/**
 * @brief Set wait-control environment variables for the target application.
 *
 * The current values are copied to caller-owned storage so they can be
 * restored after the background run finishes.
 */
static Return runit_wait_env_set(
	uint64_t              max_delay_ms,
	unsigned int          wait_point,
	int                   ready_fd,
	struct runit_wait_env *state)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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
		const enum runit_wait_env_key key = (enum runit_wait_env_key)i;

		if(RUNIT_WAIT_ENV_READY_FD == key && ready_fd < 0)
		{
			run(runit_wait_env_unset_one(runit_wait_env_names[i]));

		} else {
			const uint64_t target_value = runit_wait_env_target_value(
				key,
				max_delay_ms,
				wait_point,
				ready_fd);

			run(runit_wait_env_set_numeric(runit_wait_env_names[i],target_value));
		}
	}

	deliver(status);
}

/**
 * @brief Restore previously saved wait-control environment variable values.
 */
static Return runit_wait_env_restore(const struct runit_wait_env *state)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	for(size_t i = 0U; i < RUNIT_WAIT_ENV_COUNT; i++)
	{
		call(runit_wait_env_restore_one(
			runit_wait_env_names[i],
			state->old_value[i],
			state->had_value[i]));
	}

	deliver(status);
}

/**
 * @brief Initialize signal synchronization pipe state
 *
 * @param[out] signal_sync State to reset
 */
static void runit_signal_sync_init(struct runit_signal_sync *signal_sync)
{
	signal_sync->read_fd = RUNIT_SIGNAL_SYNC_DISABLED_FD;
	signal_sync->write_fd = RUNIT_SIGNAL_SYNC_DISABLED_FD;
	signal_sync->enabled = false;
}

/**
 * @brief Close one synchronization pipe descriptor
 *
 * @param[in,out] fd Descriptor to close and mark as disabled
 */
static void runit_signal_sync_close_fd(int *fd)
{
	if(*fd >= 0)
	{
		(void)close(*fd);
		*fd = RUNIT_SIGNAL_SYNC_DISABLED_FD;
	}
}

/**
 * @brief Close all descriptors owned by signal synchronization state
 *
 * @param[in,out] signal_sync State whose descriptors must be closed
 */
static void runit_signal_sync_cleanup(struct runit_signal_sync *signal_sync)
{
	runit_signal_sync_close_fd(&signal_sync->read_fd);
	runit_signal_sync_close_fd(&signal_sync->write_fd);
	signal_sync->enabled = false;
}

/**
 * @brief Create a pipe used to detect when the target reaches a wait point
 *
 * The pipe is created only for zero-min-delay runs. Its write descriptor is
 * inherited by the target process through the environment, while the parent
 * waits on the read descriptor before sending the requested signal
 *
 * @param[in,out] signal_sync State that receives the pipe descriptors
 * @param[in] enabled Whether synchronization is required for this run
 * @return SUCCESS when synchronization is disabled or the pipe was created
 */
static Return runit_signal_sync_prepare(
	struct runit_signal_sync *signal_sync,
	bool                     enabled)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(enabled == false)
	{
		deliver(status);
	}

	int pipe_fds[2] = {
		RUNIT_SIGNAL_SYNC_DISABLED_FD,
		RUNIT_SIGNAL_SYNC_DISABLED_FD
	};

	if(0 != pipe(pipe_fds))
	{
		echo(STDERR,"Failed to create signal wait pipe (errno %d)",errno);
		status = FAILURE;

	} else {
		signal_sync->read_fd = pipe_fds[0];
		signal_sync->write_fd = pipe_fds[1];
		signal_sync->enabled = true;
	}

	deliver(status);
}

/**
 * @brief Poll a child process before attempting signal delivery
 *
 * @param[in] app_pid Child process PID
 * @param[out] wait_status Receives child status when the child already exited
 * @param[out] child_waited Set to true when this helper consumes the child
 * @return SUCCESS when the child is still running, otherwise FAILURE
 */
static Return runit_poll_background_process_before_signal(
	pid_t app_pid,
	int   *wait_status,
	bool  *child_waited)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	pid_t poll_pid = (pid_t)-1;

	do {
		poll_pid = waitpid(app_pid,wait_status,WNOHANG);
	} while(poll_pid == (pid_t)-1 && errno == EINTR);

	if(poll_pid == app_pid)
	{
		*child_waited = true;
		echo(STDERR,"Background process exited before signal delivery");
		status = FAILURE;

	} else if(poll_pid == (pid_t)-1){
		echo(STDERR,
			"Failed to poll background PID %d before signal (errno %d)",
			(int)app_pid,
			errno);
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Wait until the child reports that the configured wait point was reached
 *
 * The wait uses bounded poll() intervals so child exit is detected promptly and
 * the same @p timeout_ms limit can report a missing wait-point notification
 *
 * @param[in] ready_fd Read descriptor of the synchronization pipe
 * @param[in] app_pid Child process PID
 * @param[in] timeout_ms Maximum time to wait for the notification
 * @param[out] wait_status Receives child status when the child already exited
 * @param[out] child_waited Set to true when this helper consumes the child
 * @return SUCCESS after a notification byte is read, otherwise FAILURE
 */
static Return runit_wait_for_signal_ready(
	int      ready_fd,
	pid_t    app_pid,
	uint64_t timeout_ms,
	int      *wait_status,
	bool     *child_waited)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	uint64_t remaining_timeout_ms = timeout_ms;

	while(SUCCESS == status && remaining_timeout_ms > 0U)
	{
		run(runit_poll_background_process_before_signal(app_pid,wait_status,child_waited));

		if(SUCCESS != status)
		{
			break;
		}

		uint64_t chunk_ms = remaining_timeout_ms;

		if(chunk_ms > 25U)
		{
			chunk_ms = 25U;
		}

		struct pollfd ready_pipe = {
			.fd = ready_fd,
			.events = POLLIN,
			.revents = 0
		};

		int poll_result = 0;

		do {
			poll_result = poll(&ready_pipe,1U,(int)chunk_ms);
		} while(poll_result == -1 && errno == EINTR);

		if(poll_result == -1)
		{
			echo(STDERR,"Failed to poll signal wait pipe (errno %d)",errno);
			status = FAILURE;

		} else if(poll_result > 0){
			if((ready_pipe.revents & POLLIN) != 0)
			{
				char notification = '\0';
				ssize_t bytes_read = 0;

				do {
					bytes_read = read(ready_fd,&notification,sizeof(notification));
				} while(bytes_read == -1 && errno == EINTR);

				if(bytes_read == (ssize_t)sizeof(notification))
				{
					deliver(status);
				}

				if(bytes_read == 0)
				{
					echo(STDERR,"Signal wait pipe closed before wait-point notification");

				} else {
					echo(STDERR,"Failed to read signal wait pipe (errno %d)",errno);
				}

				status = FAILURE;

			} else if((ready_pipe.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0){
				echo(STDERR,"Signal wait pipe failed before wait-point notification");
				status = FAILURE;
			}
		}

		remaining_timeout_ms -= chunk_ms;
	}

	if(SUCCESS == status)
	{
		echo(STDERR,"Timed out waiting for background wait-point notification");
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Send the requested signal to the background child process
 *
 * @param[in] app_pid Child process PID
 * @param[in] signal_number Signal number to send
 * @return SUCCESS when kill() reports successful delivery, otherwise FAILURE
 */
static Return runit_send_background_signal(
	pid_t app_pid,
	int   signal_number)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

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

	deliver(status);
}

/**
 * @brief Run precizer in background and complete signal-driven scenario.
 *
 * The function starts a child process, configures wait-control environment
 * variables, and sends @p signal_number to the child. When @p min_delay_ms is
 * greater than zero, delivery happens after that delay. When @p min_delay_ms is
 * zero, delivery happens immediately after the child reports that the configured
 * wait point was reached. The function then waits for completion while
 * finalizing stdout/stderr capture.
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
	const char     *arguments,
	memory         *stdout_result,
	memory         *stderr_result,
	const int      expected_return_code,
	CAPTURE_POLICY buffer_policy,
	uint64_t       min_delay_ms,
	uint64_t       max_delay_ms,
	int            signal_number,
	unsigned int   wait_point)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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

	struct runit_signal_sync signal_sync;
	runit_signal_sync_init(&signal_sync);

	const bool wait_for_ready_point = (min_delay_ms == 0U);

	call(m_del(STDOUT));
	call(m_del(STDERR));

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

	run(runit_signal_sync_prepare(&signal_sync,wait_for_ready_point));

	/* max_delay_ms also defines how long in-app wait helpers may pause. */
	run(runit_wait_env_set(
		max_delay_ms,
		wait_point,
		signal_sync.write_fd,
		&wait_environment));

	if(SUCCESS == status)
	{
		if(0 != fflush(stdout) || 0 != fflush(stderr))
		{
			serp("Failed to flush STDOUT/STDERR before forking background process");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		app_pid = fork();

		if(app_pid < 0)
		{
			serp("Failed to fork background process");
			status = FAILURE;

		} else if(0 == app_pid){
			runit_signal_sync_close_fd(&signal_sync.read_fd);

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
				runit_signal_sync_close_fd(&signal_sync.write_fd);
				run_watchdog(protected_pid,max_delay_ms,25U);
			}

			if(EXTERNAL_CALL == run_external)
			{
				(void)execv(runit_call_data.program_path,runit_call_data.argv);
				_exit(127);
			}

			const testitall_test_main_callback internal_test_main = testitall_get_test_main();
			if(NULL == internal_test_main)
			{
				_exit(127);
			}

			const int test_main_result = internal_test_main((int)runit_call_data.argc,runit_call_data.argv);
			fflush(stdout);
			fflush(stderr);
			_exit(test_main_result);

		} else {
			child_started = true;
			runit_signal_sync_close_fd(&signal_sync.write_fd);
			/* Parent keeps paths only; capture fds are no longer needed in this process. */
			runit_capture_close_fds(&capture);
		}
	}

	if(SUCCESS == status)
	{
		if(wait_for_ready_point == true)
		{
			run(runit_wait_for_signal_ready(
				signal_sync.read_fd,
				app_pid,
				max_delay_ms,
				&wait_status,
				&child_waited));

		} else {
			/* Give the app time to reach signal_wait_at_point(), then inject the requested signal. */
			sleep_for_milliseconds(min_delay_ms);
			run(runit_poll_background_process_before_signal(
				app_pid,
				&wait_status,
				&child_waited));
		}
	}

	if(SUCCESS == status)
	{
		run(runit_send_background_signal(app_pid,signal_number));
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
	runit_signal_sync_cleanup(&signal_sync);
	runit_release_call_and_capture(&capture,&runit_call_data);

	run(m_del(STDERR));

	deliver(status);
}
