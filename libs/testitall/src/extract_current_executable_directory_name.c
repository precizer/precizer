#include "testitall.h"
#include <unistd.h>
#include <string.h>
#include <limits.h>
#ifdef EVIL_EMPIRE
/* macOS build flag */
#include <mach-o/dyld.h>
#endif

/** @brief Extract the last directory name from the directory containing the executable
 *
 * This function reads the path to the currently running executable from /proc/self/exe,
 * identifies the parent directory of the binary, and extracts the last directory name
 * from that path.
 *
 * Example:
 *   /proc/self/exe -> "../.builds/testitall/sanitize/testitall"
 *   -> directory of executable = "../.builds/testitall/sanitize"
 *   -> extracted = "sanitize"
 *
 * The output is written into 'environment' buffer (on the caller stack).
 *
 * @param environment Pointer to the caller's stack buffer
 * @param environment_size Size of this buffer in bytes
 * @return Return status code (SUCCESS on success, FAILURE on error)
 */
Return extract_current_executable_directory_name(
	char   *environment,
	size_t environment_size)
{
	/** Return status
	 *  The status that will be passed to provide() before exiting
	 *  By default, the function worked without errors
	 */
	Return status = SUCCESS;

	char exe_path[PATH_MAX];
	ssize_t len = 0;
	const char *last_slash = NULL;
	const char *before_last_slash = NULL;
	const char *walker = NULL;
	const char *directory_start = NULL;
	size_t directory_length = 0U;

	if(NULL == environment)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(0U == environment_size)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
#ifdef EVIL_EMPIRE
		/* macOS-specific executable path lookup */
		uint32_t bufsize = (uint32_t)sizeof(exe_path);

		/* _NSGetExecutablePath returns 0 on success, else sets required size. */
		if(0 != _NSGetExecutablePath(exe_path,&bufsize))
		{
			status = FAILURE;
		} else {
			char resolved[PATH_MAX];
			if(NULL == realpath(exe_path,resolved))
			{
				status = FAILURE;
			} else {
				strncpy(exe_path,resolved,sizeof(exe_path) - 1U);
				exe_path[sizeof(exe_path) - 1U] = '\0';
				len = (ssize_t)strlen(exe_path);
			}
		}
#else
		len = readlink("/proc/self/exe",exe_path,(size_t)PATH_MAX - 1U);

		if(len < 0)
		{
			status = FAILURE;
		} else {
			/* If the returned length hits the limit, the path could be truncated */
			if(len >= (ssize_t)((size_t)PATH_MAX - 1U))
			{
				status = FAILURE;
			}
		}
#endif
	}

	if(SUCCESS == status)
	{
		exe_path[len] = '\0';

		/* Identify last and previous '/' */
		walker = exe_path;

		while('\0' != *walker)
		{
			if('/' == *walker)
			{
				before_last_slash = last_slash;
				last_slash = walker;
			}
			walker++;
		}

		if(NULL == last_slash)
		{
			/* Should never happen for /proc/self/exe */
			if(environment_size > 0U)
			{
				environment[0] = '\0';
			} else {
				status = FAILURE;
			}
		} else {
			/* Parent directory ends at last_slash */
			if(NULL == before_last_slash)
			{
				/* Path like "/binary": parent dir is "/" and has no name */
				if(environment_size > 0U)
				{
					environment[0] = '\0';
				} else {
					status = FAILURE;
				}
			} else {
				/* Directory name starts after before_last_slash */
				directory_start = before_last_slash + 1;
				directory_length = (size_t)(last_slash - directory_start);

				if(directory_length + 1U > environment_size)
				{
					status = FAILURE;
				} else {
					if(directory_length > 0U)
					{
						memcpy(environment,directory_start,directory_length);
						environment[directory_length] = '\0';
					} else {
						environment[0] = '\0';
					}
				}
			}
		}
	}

	provide(status);
}
