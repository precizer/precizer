#include "testitall.h"

#include "runit_internal.h"

enum run_mode run_external = EXTERNAL_CALL;

/**
 * @brief Process-state guard used for INTERNAL_TEST in-process execution.
 */
struct runit_internal_guard {
	char *previous_cwd;
	bool previous_cwd_captured;
	bool changed_directory;
	bool stdio_saved;
	int saved_stdout_fd;
	int saved_stderr_fd;
};

/**
 * @brief Initialize the INTERNAL_TEST process-state guard.
 */
static void runit_internal_guard_init(struct runit_internal_guard *guard)
{
	guard->previous_cwd = NULL;
	guard->previous_cwd_captured = false;
	guard->changed_directory = false;
	guard->stdio_saved = false;
	guard->saved_stdout_fd = -1;
	guard->saved_stderr_fd = -1;
}

/**
 * @brief Enter INTERNAL_TEST mode execution context.
 *
 * Captures current directory, switches to TMPDIR, saves stdout/stderr descriptors,
 * and redirects process output to capture files.
 */
static Return runit_internal_enter(
	struct runit_internal_guard  *guard,
	const char                   *tmpdir,
	struct runit_capture_session *capture)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	guard->previous_cwd = getcwd(NULL,0);

	if(NULL == guard->previous_cwd)
	{
		serp("Failed to get current working directory");
		status = FAILURE;

	} else {
		guard->previous_cwd_captured = true;
	}

	if(SUCCESS == status)
	{
		if(0 != chdir(tmpdir))
		{
			serp("Failed to change directory to TMPDIR");
			status = FAILURE;

		} else {
			guard->changed_directory = true;
		}
	}

	if(SUCCESS == status)
	{
		if(0 != fflush(stdout) || 0 != fflush(stderr))
		{
			serp("Failed to flush STDOUT/STDERR before redirect");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		guard->saved_stdout_fd = dup(STDOUT_FILENO);
		guard->saved_stderr_fd = dup(STDERR_FILENO);

		if(guard->saved_stdout_fd == -1 || guard->saved_stderr_fd == -1)
		{
			serp("Failed to save stdout/stderr descriptors");
			status = FAILURE;

		} else {
			guard->stdio_saved = true;
		}
	}

	if(SUCCESS == status)
	{
		/* Redirect in-process stdout/stderr into the same capture files as external mode. */
		run(runit_capture_apply_redirect(capture));
	}

	deliver(status);
}

/**
 * @brief Leave INTERNAL_TEST mode execution context and restore process state.
 */
static Return runit_internal_leave(struct runit_internal_guard *guard)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(true == guard->stdio_saved)
	{
		if(guard->saved_stdout_fd != -1 && dup2(guard->saved_stdout_fd,STDOUT_FILENO) == -1)
		{
			serp("Failed to restore original STDOUT");
			status = FAILURE;
		}

		if(guard->saved_stderr_fd != -1 && dup2(guard->saved_stderr_fd,STDERR_FILENO) == -1)
		{
			serp("Failed to restore original STDERR");
			status = FAILURE;
		}
	}

	if(guard->saved_stdout_fd != -1)
	{
		(void)close(guard->saved_stdout_fd);
		guard->saved_stdout_fd = -1;
	}

	if(guard->saved_stderr_fd != -1)
	{
		(void)close(guard->saved_stderr_fd);
		guard->saved_stderr_fd = -1;
	}

	if(true == guard->changed_directory && true == guard->previous_cwd_captured)
	{
		if(0 != chdir(guard->previous_cwd))
		{
			serp("Failed to restore working directory");
			status = FAILURE;
		}
	}

	if(true == guard->previous_cwd_captured)
	{
		free(guard->previous_cwd);
		guard->previous_cwd = NULL;
		guard->previous_cwd_captured = false;
	}

	guard->changed_directory = false;
	guard->stdio_saved = false;

	deliver(status);
}

/**
 * @brief Run precizer with arguments in-process or via external command.
 *
 * @param arguments Command-line arguments without the binary name.
 * @param stdout_result Buffer to receive stdout (NULL to ignore).
 * @param stderr_result Buffer to receive stderr (NULL to ignore).
 * @param expected_return_code Expected exit code from the run.
 * @param buffer_policy Bitmask controlling stdout/stderr handling (see capture_policy).
 *
 * @note Internal helper-to-helper calls inside this closed API use lightweight
 * argument checks by design to improve readability and reduce overhead.
 * Keep this approach in future runit* internal helpers as well: avoid redundant
 * argument validation in internal-only functions that are not externally visible
 * and exist to remove code duplication.
 *
 * @return SUCCESS when the run completes with the expected exit code; FAILURE otherwise.
 */
Return runit(
	const char   *arguments,
	memory       *stdout_result,
	memory       *stderr_result,
	const int    expected_return_code,
	CAPTURE_POLICY buffer_policy)
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

	struct runit_internal_guard internal_guard;
	runit_internal_guard_init(&internal_guard);

	int wait_status = 0;
	bool child_waited = false;
	Return finalize_status = SUCCESS;

	call(m_del(STDOUT));
	call(m_del(STDERR));

	run(runit_validate_runtime_mode(run_external));

	const char *call_label = "Internal call";

	if(EXTERNAL_CALL == run_external)
	{
		call_label = "External call";
	}
	run(runit_prepare_call_and_capture(
		&runit_call_data,
		&capture,
		run_external,
		safe_arguments,
		call_label,
		stdout_result,
		stderr_result,
		expected_return_code,
		buffer_policy));

	if(SUCCESS == status && EXTERNAL_CALL == run_external)
	{
		const pid_t app_pid = fork();

		if(app_pid < 0)
		{
			serp("Failed to fork process for EXTERNAL_CALL");
			status = FAILURE;

		} else if(0 == app_pid){
			if(chdir(runit_call_data.tmpdir) != 0)
			{
				_exit(127);
			}

			if(SUCCESS != runit_capture_apply_redirect(&capture))
			{
				_exit(127);
			}

			(void)execv(runit_call_data.program_path,runit_call_data.argv);
			_exit(127);

		} else {
			runit_capture_close_fds(&capture);

			run(runit_wait_child(app_pid,&wait_status,"EXTERNAL_CALL"));

			if(SUCCESS == status)
			{
				child_waited = true;
			}
		}
	}

	if(INTERNAL_TEST == run_external)
	{
		const testitall_test_main_callback internal_test_main = testitall_get_test_main();

		run(runit_internal_enter(&internal_guard,runit_call_data.tmpdir,&capture));

		if(SUCCESS == status && NULL != internal_test_main)
		{
			const int test_main_result = internal_test_main((int)runit_call_data.argc,runit_call_data.argv);
			fflush(stdout);
			fflush(stderr);

			wait_status = (test_main_result & 0xff) << 8;
			child_waited = true;
		}

		call(runit_internal_leave(&internal_guard));
	}

	if(true == child_waited)
	{
		finalize_status = runit_capture_finalize(&capture,wait_status);
	}

	if(SUCCESS == status && SUCCESS != finalize_status)
	{
		status = finalize_status;
	}

	runit_release_call_and_capture(&capture,&runit_call_data);

	run(m_del(STDERR));

	deliver(status);
}
