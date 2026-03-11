#include "testitall.h"
#include <unistd.h>
#include <string.h>
#include <limits.h>
#ifdef EVIL_EMPIRE_OS // macOS build flag
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#include <mach-o/dyld.h>
#endif

/**
 * @brief Extract the name of the directory that contains the current executable
 *
 * This function determines the directory that contains the currently running
 * executable and writes only that directory name into @p environment
 *
 * In this repository the returned value is used as the build configuration
 * name, for example `debug`, `sanitize`, or `coverage`
 *
 * Example:
 *   executable path = "/worktree/.builds/testitall/debug/testitall"
 *   containing directory path = "/worktree/.builds/testitall/debug"
 *   returned directory name = "debug"
 *
 * A path like "/binary" yields an empty string because the containing
 * directory is the root path "/"
 *
 * @param environment Output memory descriptor initialized for char elements
 * @return SUCCESS on success, FAILURE on error
 */
Return extract_current_executable_directory_name(
	memory *environment)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *executable_path = NULL;

	if(SUCCESS == status)
	{
		if(SUCCESS != resize(environment,(size_t)PATH_MAX))
		{
			status = FAILURE;
		} else {
			executable_path = data(char,environment);

			if(NULL == executable_path)
			{
				status = FAILURE;
			}
		}
	}

#ifdef EVIL_EMPIRE_OS // macOS build flag
	if(SUCCESS == status)
	{
		create(char,executable_path_source_buffer);
		uint32_t executable_path_size = (uint32_t)PATH_MAX;
		char *executable_path_source = NULL;

		if(SUCCESS != resize(executable_path_source_buffer,(size_t)PATH_MAX))
		{
			status = FAILURE;
		} else {
			executable_path_source = data(char,executable_path_source_buffer);

			if(NULL == executable_path_source)
			{
				status = FAILURE;
			}
		}

		/* _NSGetExecutablePath returns 0 on success, else sets required size */
		if(SUCCESS == status && 0 != _NSGetExecutablePath(executable_path_source,&executable_path_size))
		{
			status = FAILURE;
		}

		if(SUCCESS == status && NULL == realpath(executable_path_source,executable_path))
		{
			status = FAILURE;
		}

		call(del(executable_path_source_buffer));
	}

#else

	if(SUCCESS == status)
	{
		ssize_t executable_path_length = 0;

		executable_path_length = readlink("/proc/self/exe",executable_path,(size_t)PATH_MAX - 1U);

		if(executable_path_length < 0)
		{
			status = FAILURE;
		} else {
			/* If the returned length hits the limit, the path could be truncated */
			if(executable_path_length >= (ssize_t)((size_t)PATH_MAX - 1U))
			{
				status = FAILURE;
			} else {
				executable_path[executable_path_length] = '\0';
			}
		}
	}
#endif

	if(SUCCESS == status)
	{
		const char *last_slash = NULL;
		const char *previous_slash = NULL;
		const char *walker = executable_path;

		/* Identify last and previous '/' */
		while('\0' != *walker)
		{
			if('/' == *walker)
			{
				previous_slash = last_slash;
				last_slash = walker;
			}
			walker++;
		}

		if(NULL == last_slash)
		{
			status = FAILURE;
		} else {
			/* Parent directory ends at last_slash */
			if(NULL == previous_slash)
			{
				/* Path like "/binary": parent dir is "/" and has no name */
				executable_path[0] = '\0';

				if(SUCCESS != resize(environment,1U,RELEASE_UNUSED))
				{
					status = FAILURE;
				}
			} else {
				/* Directory name starts after previous_slash */
				const char *directory_name = previous_slash + 1;
				const size_t directory_length = (size_t)(last_slash - directory_name);

				memmove(executable_path,directory_name,directory_length);
				executable_path[directory_length] = '\0';

				if(SUCCESS != resize(environment,directory_length + 1U,RELEASE_UNUSED))
				{
					status = FAILURE;
				}
			}
		}
	}

	if(SUCCESS != status)
	{
		del(environment);
	}

	deliver(status);
}
