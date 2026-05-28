#include "runit_internal.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

static char runit_internal_program_name[] = "precizer";

/**
 * @brief Initialize runit_call fields and owned memory descriptors.
 */
void runit_call_init(struct runit_call *call)
{
	memset(call,0,sizeof(*call));
	call->program_path_buffer = m_init(char,MEMORY_STRING);
	call->argv_buffer = m_init(char *);
}

/**
 * @brief Free any resources acquired by runit_call_prepare().
 */
void runit_call_cleanup(struct runit_call *call)
{
	if(true == call->words_allocated)
	{
		wordfree(&call->parsed_arguments);
	}

	(void)m_del(&call->argv_buffer);
	(void)m_del(&call->program_path_buffer);

	runit_call_init(call);
}

Return runit_reset_result_buffers(
	memory *stdout_result,
	memory *stderr_result)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(NULL != stdout_result)
	{
		call(m_del(stdout_result));
	}

	if(NULL != stderr_result)
	{
		call(m_del(stderr_result));
	}

	deliver(status);
}

/**
 * @brief Validate run mode for public runit* API entrypoints.
 */
Return runit_validate_runtime_mode(enum run_mode mode)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(EXTERNAL_CALL != mode && INTERNAL_TEST != mode)
	{
		echo(STDERR,"Unsupported run_external mode: %d",(int)mode);
		status = FAILURE;
	}

	if(SUCCESS == status && INTERNAL_TEST == mode && NULL == test_main)
	{
		echo(STDERR,"test_main symbol is not available for INTERNAL_TEST mode");
		status = FAILURE;
	}

	deliver(status);
}

/**
 * @brief Prepare executable path and argv for a runit-style call.
 *
 * For EXTERNAL_CALL mode, resolves `${BINDIR}/precizer`.
 * For INTERNAL_TEST mode, uses "precizer" as argv[0].
 * Mode validity is checked by public runit* entrypoints.
 */
Return runit_call_prepare(
	struct runit_call *call,
	enum run_mode     mode,
	const char        *arguments)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *bindir = NULL;
	const bool is_external_mode = (EXTERNAL_CALL == mode);

	call->safe_arguments = arguments;

	if(SUCCESS == status)
	{
		call->tmpdir = getenv("TMPDIR");

		if(NULL == call->tmpdir)
		{
			echo(STDERR,"Environment variable TMPDIR is not set");
			status = FAILURE;
		}
	}

	if(SUCCESS == status && true == is_external_mode)
	{
		bindir = getenv("BINDIR");

		if(NULL == bindir)
		{
			echo(STDERR,"Environment variable BINDIR is not set");
			status = FAILURE;
		}
	}

	if(SUCCESS == status && true == is_external_mode)
	{
		const char suffix[] = "/precizer";
		const size_t bindir_length = strlen(bindir);
		const size_t suffix_length = sizeof(suffix);

		if(bindir_length > SIZE_MAX - suffix_length)
		{
			echo(STDERR,"The BINDIR path is too long");
			status = FAILURE;

		} else {
			const size_t path_size = bindir_length + suffix_length;

			if(SUCCESS != m_resize(&call->program_path_buffer,path_size))
			{
				report("Memory allocation failed, requested size: %zu bytes",path_size);
				status = FAILURE;

			} else {
				call->program_path = m_data(char,&call->program_path_buffer);

				if(NULL == call->program_path)
				{
					status = FAILURE;

				} else {
					const int formatted_path_length = snprintf(call->program_path,path_size,"%s%s",bindir,suffix);

					if(formatted_path_length < 0)
					{
						echo(STDERR,"Failed to build executable path from BINDIR");
						status = FAILURE;

					} else {
						run(m_finalize_string(&call->program_path_buffer,(size_t)formatted_path_length));
					}
				}
			}
		}
	}

	if(SUCCESS == status && false == is_external_mode)
	{
		call->program_path = runit_internal_program_name;
	}

	if(SUCCESS == status)
	{
		const int word_status = wordexp(call->safe_arguments,&call->parsed_arguments,WRDE_NOCMD);

		if(0 == word_status)
		{
			call->words_allocated = true;

		} else {
			if(WRDE_NOSPACE == word_status)
			{
				/*
				 * wordexp() may allocate partially on WRDE_NOSPACE and requires
				 * wordfree() in cleanup.
				 */
				call->words_allocated = true;
			}

			echo(STDERR,
				"Failed to parse arguments \"%s\" (wordexp code %d)",
				call->safe_arguments,
				word_status);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(call->parsed_arguments.we_wordc > ((size_t)INT_MAX - 1U))
		{
			echo(STDERR,"Too many arguments: %zu",call->parsed_arguments.we_wordc);
			status = FAILURE;

		} else {
			call->argc = call->parsed_arguments.we_wordc + 1U;
		}
	}

	if(SUCCESS == status)
	{
		const size_t argv_size = call->argc + 1U;

		if(SUCCESS != m_resize(&call->argv_buffer,argv_size,ZERO_NEW_MEMORY))
		{
			report("Memory allocation failed, requested size: %zu bytes",argv_size * sizeof(char *));
			status = FAILURE;

		} else {
			call->argv = m_data(char *,&call->argv_buffer);

			if(NULL == call->argv)
			{
				status = FAILURE;

			} else {
				call->argv[0] = call->program_path;

				for(size_t i = 0U; i < call->parsed_arguments.we_wordc; i++)
				{
					call->argv[i + 1U] = call->parsed_arguments.we_wordv[i];
				}
				call->argv[call->argc] = NULL;
			}
		}
	}

	deliver(status);
}

Return runit_prepare_call_and_capture(
	struct runit_call            *call,
	struct runit_capture_session *capture,
	enum run_mode                mode,
	const char                   *arguments,
	const char                   *call_label,
	memory                       *stdout_result,
	memory                       *stderr_result,
	int                          expected_return_code,
	CAPTURE_POLICY               buffer_policy)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	run(runit_reset_result_buffers(stdout_result,stderr_result));
	run(runit_call_prepare(call,mode,arguments));
	run(runit_capture_prepare(
		capture,
		call->tmpdir,
		call->safe_arguments,
		call_label,
		stdout_result,
		stderr_result,
		expected_return_code,
		buffer_policy));

	deliver(status);
}

void runit_release_call_and_capture(
	struct runit_capture_session *capture,
	struct runit_call            *call)
{
	runit_capture_cleanup(capture);
	runit_call_cleanup(call);
}

/**
 * @brief Wait for a child PID and retry waitpid() on EINTR.
 */
Return runit_wait_child(
	pid_t      child_pid,
	int        *wait_status,
	const char *context_label)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	pid_t waited_pid = (pid_t)-1;

	do {
		waited_pid = waitpid(child_pid,wait_status,0);
	} while(waited_pid == (pid_t)-1 && errno == EINTR);

	if(waited_pid == (pid_t)-1)
	{
		echo(STDERR,"Failed to waitpid for %s (errno %d)",context_label,errno);
		status = FAILURE;
	}

	deliver(status);
}
