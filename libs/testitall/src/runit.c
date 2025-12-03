#include "testitall.h"
#include <limits.h>

#ifndef ARG_MAX
#ifdef _POSIX_ARG_MAX
#define ARG_MAX _POSIX_ARG_MAX
#else
#define ARG_MAX 4096
#endif
#endif

Return runit(
	const char *arguments,
	const int  expected_return_code,
	bool       suppress_stderr,
	bool       suppress_stdout)
{
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	const char *command_prefix = "export TESTING=true && cd ${TMPDIR} && ${BINDIR}/precizer";
	const char *safe_arguments = arguments;

	if(NULL == safe_arguments)
	{
		safe_arguments = "";
	}

	char command[ARG_MAX];
	int written = snprintf(
		command,
		sizeof(command),
		"%s %s",
		command_prefix,
		safe_arguments);

	if(written < 0 || (size_t)written >= sizeof(command))
	{
		echo(STDERR,"Command length exceeds ARG_MAX (%zu bytes)",sizeof(command));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = external_call(command,
			expected_return_code,
			suppress_stderr,
			suppress_stdout);
	}

	provide(status);
}
