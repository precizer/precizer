#include "runit_internal.h"

/**
 * @brief Close file descriptor if it is valid and reset it to -1.
 */
static void close_fd_if_open(int *fd)
{
	if(*fd >= 0)
	{
		(void)close(*fd);
		*fd = -1;
	}
}

/**
 * @brief Create one temporary capture file with mkstemp().
 *
 * The created path is stored in @p path and descriptor in @p fd.
 */
static Return create_capture_file(
	const char *tmpdir,
	const char *kind,
	char       *path,
	size_t      path_size,
	int        *fd)
{
	Return status = SUCCESS;

	int written = snprintf(path,path_size,"%s/testitall.%s.XXXXXX",tmpdir,kind);
	if(written < 0 || (size_t)written >= path_size)
	{
		echo(STDERR,"Failed to build a temporary capture path for %s",kind);
		status = FAILURE;
		provide(status);
	}

	*fd = mkstemp(path);
	if(*fd < 0)
	{
		serp("Failed to create temporary capture file");
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Load full capture-file content into a libmem buffer.
 */
static Return load_capture_file(
	const char *path,
	memory     *buffer)
{
	Return status = SUCCESS;
	FILE *file = NULL;
	long file_size_long = 0;
	size_t file_size = 0U;

	call(del(buffer));

	file = fopen(path,"rb");
	if(NULL == file)
	{
		serp("Failed to open capture file");
		status = FAILURE;
		provide(status);
	}

	if(0 != fseek(file,0,SEEK_END))
	{
		serp("Failed to seek capture file");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		file_size_long = ftell(file);
		if(file_size_long < 0)
		{
			serp("Failed to get capture file size");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		file_size = (size_t)file_size_long;
	}

	if(SUCCESS == status)
	{
		if(0 != fseek(file,0,SEEK_SET))
		{
			serp("Failed to rewind capture file");
			status = FAILURE;
		}
	}

	if(SUCCESS == status && file_size > 0U)
	{
		run(resize(buffer,file_size + 1U));
	}

	if(SUCCESS == status && file_size > 0U)
	{
		char *buffer_data = data(char,buffer);
		size_t bytes_read = fread(buffer_data,1,file_size,file);
		if(bytes_read != file_size)
		{
			echo(STDERR,"Failed to read capture file: %s",path);
			status = FAILURE;
		} else {
			buffer_data[file_size] = '\0';
		}
	}

	if(NULL != file)
	{
		(void)fclose(file);
	}

	if(SUCCESS != status)
	{
		call(del(buffer));
	}

	provide(status);
}

/**
 * @brief Replace global STDERR buffer with formatted message text.
 */
static Return set_stderr_message(
	char *message,
	bool  fail_after_write)
{
	Return status = SUCCESS;

	run(copy_literal(STDERR,message));

	free(message);

	if(SUCCESS == status && true == fail_after_write)
	{
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Format detailed failure report for unexpected process exit.
 */
static Return format_unexpected_exit_report(
	const struct runit_capture_session *session,
	int                                 wait_status)
{
	Return status = SUCCESS;

	const bool exited_normally = WIFEXITED(wait_status);
	int exit_code = -1;
	if(true == exited_normally)
	{
		exit_code = WEXITSTATUS(wait_status);
	}

	int term_signal = 0;
	if(WIFSIGNALED(wait_status))
	{
		term_signal = WTERMSIG(wait_status);
	}
	const char *stderr_view = getcstring(STDERR);
	const char *stdout_view = getcstring(STDOUT);

	char *message = NULL;
	int written = asprintf(
		&message,
		YELLOW "ERROR: Unexpected exit code!" RESET "\n"
		YELLOW "%s:\n" YELLOW ">>" RESET "precizer %s" YELLOW "<<" RESET "\n"
		YELLOW "Exited with code " RESET "%d" YELLOW " but expected " RESET "%d\n"
		YELLOW "Process terminated signal" RESET " %d\n"
		YELLOW "Stderr output:\n>>" RESET "%s" YELLOW "<<" RESET "\n"
		YELLOW "Stdout output:\n>>" RESET "%s" YELLOW "<<" RESET "\n",
		session->call_label,
		session->arguments,
		exit_code,
		session->expected_return_code,
		term_signal,
		stderr_view,
		stdout_view);

	if(written < 0)
	{
		report("Memory allocation failed while formatting unexpected exit report");
		status = FAILURE;
		provide(status);
	}

	run(set_stderr_message(message,true));

	provide(status);
}

/**
 * @brief Format warning report when stderr is not allowed but contains data.
 */
static Return format_stderr_warning_report(const struct runit_capture_session *session)
{
	Return status = SUCCESS;

	const char *stderr_view = getcstring(STDERR);

	char *message = NULL;
	int written = asprintf(
		&message,
		YELLOW "Warning! STDERR buffer is not empty!\n"
		"%s:\n" YELLOW ">>" RESET "precizer %s" YELLOW "<<" RESET "\n"
		"Stderr output:\n" YELLOW ">>" RESET "%s" YELLOW "<<" RESET "\n",
		session->call_label,
		session->arguments,
		stderr_view);

	if(written < 0)
	{
		report("Memory allocation failed while formatting stderr warning report");
		status = FAILURE;
		provide(status);
	}

	run(set_stderr_message(message,true));

	provide(status);
}

/**
 * @brief Initialize capture session fields to a clean default state.
 */
void runit_capture_session_init(struct runit_capture_session *session)
{
	memset(session,0,sizeof(*session));
	session->stdout_fd = -1;
	session->stderr_fd = -1;
}

/**
 * @brief Close any opened capture file descriptors in a session.
 */
void runit_capture_close_fds(struct runit_capture_session *session)
{
	close_fd_if_open(&session->stdout_fd);
	close_fd_if_open(&session->stderr_fd);
}

/**
 * @brief Allocate and prepare capture files and session metadata.
 */
Return runit_capture_prepare(
	struct runit_capture_session *session,
	const char                   *tmpdir,
	const char                   *arguments,
	const char                   *call_label,
	memory                       *stdout_result,
	memory                       *stderr_result,
	int                           expected_return_code,
	unsigned int                  buffer_policy)
{
	Return status = SUCCESS;

	session->stdout_result = stdout_result;
	session->stderr_result = stderr_result;
	session->expected_return_code = expected_return_code;
	session->buffer_policy = buffer_policy;
	session->call_label = call_label;
	session->arguments = arguments;

	run(create_capture_file(tmpdir,"stdout",
		session->stdout_path,sizeof(session->stdout_path),&session->stdout_fd));

	run(create_capture_file(tmpdir,"stderr",
		session->stderr_path,sizeof(session->stderr_path),&session->stderr_fd));

	if(SUCCESS != status)
	{
		runit_capture_cleanup(session);
	}

	provide(status);
}

/**
 * @brief Redirect process stdout/stderr to prepared capture files.
 */
Return runit_capture_apply_redirect(struct runit_capture_session *session)
{
	Return status = SUCCESS;

	if(dup2(session->stdout_fd,STDOUT_FILENO) == -1)
	{
		serp("Failed to redirect STDOUT to capture file");
		status = FAILURE;
	}

	if(SUCCESS == status && dup2(session->stderr_fd,STDERR_FILENO) == -1)
	{
		serp("Failed to redirect STDERR to capture file");
		status = FAILURE;
	}

	runit_capture_close_fds(session);

	provide(status);
}

/**
 * @brief Validate child wait status against expected return code.
 */
static Return runit_capture_validate_exit_status(
	const struct runit_capture_session *session,
	int                                 wait_status)
{
	Return status = SUCCESS;

	if(!WIFEXITED(wait_status) || session->expected_return_code != WEXITSTATUS(wait_status))
	{
		/* Exit-code mismatch has priority and rewrites STDERR with full diagnostics. */
		run(format_unexpected_exit_report(session,wait_status));
	}

	provide(status);
}

/**
 * @brief Apply stderr policy when captured stderr is present.
 */
static Return runit_capture_apply_stderr_policy(const struct runit_capture_session *session)
{
	Return status = SUCCESS;
	const bool suppress_stderr = (session->buffer_policy & STDERR_SUPPRESS) != 0U;
	const bool allow_stderr = (session->buffer_policy & STDERR_ALLOW) != 0U;

	if(STDERR->length > 0U)
	{
		if(true == allow_stderr)
		{
			/* Keep STDERR as-is and do not fail. */

		} else if(true == suppress_stderr) {
			call(del(STDERR));

		} else {
			run(format_stderr_warning_report(session));
		}
	}

	provide(status);
}

/**
 * @brief Apply stdout suppression policy.
 */
static Return runit_capture_apply_stdout_policy(const struct runit_capture_session *session)
{
	Return status = SUCCESS;
	const bool suppress_stdout = (session->buffer_policy & STDOUT_SUPPRESS) != 0U;

	if(STDOUT->length > 0U && true == suppress_stdout)
	{
		call(del(STDOUT));
	}

	provide(status);
}

/**
 * @brief Copy captured output into caller-provided buffers.
 */
static Return runit_capture_copy_results(const struct runit_capture_session *session)
{
	Return status = SUCCESS;

	if(NULL != session->stdout_result && STDOUT->length > 0U)
	{
		call(copy(session->stdout_result,STDOUT));
	}

	if(NULL != session->stderr_result && STDERR->length > 0U)
	{
		call(copy(session->stderr_result,STDERR));
	}

	provide(status);
}

/**
 * @brief Finalize captures, validate exit status, and return requested buffers.
 */
Return runit_capture_finalize(
	struct runit_capture_session *session,
	int                           wait_status)
{
	Return status = SUCCESS;
	const bool allow_stderr = (session->buffer_policy & STDERR_ALLOW) != 0U;

	call(del(STDOUT));
	call(del(STDERR));

	call(load_capture_file(session->stdout_path,STDOUT));
	call(load_capture_file(session->stderr_path,STDERR));

	run(runit_capture_validate_exit_status(session,wait_status));
	run(runit_capture_apply_stderr_policy(session));
	call(runit_capture_apply_stdout_policy(session));
	call(runit_capture_copy_results(session));

	if(true == allow_stderr)
	{
		run(del(STDERR));
	}

	call(del(STDOUT));

	provide(status);
}

/**
 * @brief Remove temporary capture artifacts and reset session state.
 */
void runit_capture_cleanup(struct runit_capture_session *session)
{
	runit_capture_close_fds(session);

	if('\0' != session->stdout_path[0])
	{
		(void)unlink(session->stdout_path);
	}

	if('\0' != session->stderr_path[0])
	{
		(void)unlink(session->stderr_path);
	}

	runit_capture_session_init(session);
}
