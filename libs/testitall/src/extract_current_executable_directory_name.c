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
Return extract_current_executable_directory_name(memory *environment)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *executable_path_data_rewritable = NULL;
	size_t executable_path_length = 0U;
	size_t directory_result_length = 0U;

	if(TRIUMPH & status)
	{
		status = m_resize(environment,(size_t)PATH_MAX);
	}

	if(TRIUMPH & status)
	{
		executable_path_data_rewritable = m_data(char,environment);

		if(NULL == executable_path_data_rewritable)
		{
			status = FAILURE;
		}
	}

#ifdef EVIL_EMPIRE_OS // macOS build flag
	if(TRIUMPH & status)
	{
		m_create(char,executable_path_source_buffer,MEMORY_STRING);
		uint32_t executable_path_size = (uint32_t)PATH_MAX;
		char *executable_path_source_data_rewritable = NULL;

		if(TRIUMPH & status)
		{
			status = m_resize(executable_path_source_buffer,(size_t)PATH_MAX);
		}

		if(TRIUMPH & status)
		{
			executable_path_source_data_rewritable = m_data(char,executable_path_source_buffer);

			if(NULL == executable_path_source_data_rewritable)
			{
				status = FAILURE;
			}
		}

		/* _NSGetExecutablePath returns 0 on success, else sets required size */
		if((TRIUMPH & status) && 0 != _NSGetExecutablePath(executable_path_source_data_rewritable,&executable_path_size))
		{
			status = FAILURE;
		}

		if((TRIUMPH & status) && NULL == realpath(executable_path_source_data_rewritable,executable_path_data_rewritable))
		{
			status = FAILURE;
		}

		if(TRIUMPH & status)
		{
			executable_path_length = strnlen(executable_path_data_rewritable,(size_t)PATH_MAX);

			if(executable_path_length >= (size_t)PATH_MAX)
			{
				status = FAILURE;
			}
		}

		call(m_del(executable_path_source_buffer));
	}
#else
	if(TRIUMPH & status)
	{
		ssize_t readlink_length = 0;

		readlink_length = readlink("/proc/self/exe",executable_path_data_rewritable,(size_t)PATH_MAX - 1U);

		if(readlink_length < 0)
		{
			status = FAILURE;
		} else {
			/* If the returned length hits the limit, the path could be truncated */
			if(readlink_length >= (ssize_t)((size_t)PATH_MAX - 1U))
			{
				status = FAILURE;
			} else {
				executable_path_length = (size_t)readlink_length;
			}
		}
	}
#endif

	if(TRIUMPH & status)
	{
		status = m_finalize_string(environment,executable_path_length,WRITE_TERMINATOR_ALWAYS);
	}

	if(TRIUMPH & status)
	{
		const char *last_slash = NULL;
		const char *previous_slash = NULL;
		const char *executable_path_text = m_text(environment);

		/* Identify last and previous '/' */
		for(size_t i = 0U; i < executable_path_length; i++)
		{
			if('/' == executable_path_text[i])
			{
				previous_slash = last_slash;
				last_slash = &executable_path_text[i];
			}
		}

		if(NULL == last_slash)
		{
			status = FAILURE;
		} else {
			/* Parent directory ends at last_slash */
			if(NULL == previous_slash)
			{
				/* Path like "/binary": parent dir is "/" and has no name */
				executable_path_data_rewritable[0] = '\0';

				status = m_finalize_string(environment,0U,WRITE_TERMINATOR_ALWAYS);

			} else {
				/* Directory name starts after previous_slash */
				const char *directory_name = previous_slash + 1;
				const size_t directory_offset = (size_t)(directory_name - executable_path_text);
				const size_t directory_length = (size_t)(last_slash - directory_name);

				memmove(
					executable_path_data_rewritable,
					executable_path_data_rewritable + directory_offset,
					directory_length);

				status = m_finalize_string(environment,directory_length,WRITE_TERMINATOR_ALWAYS);
			}
		}
	}

	if(TRIUMPH & status)
	{
		status = m_string_length(environment,&directory_result_length);
	}

	if(TRIUMPH & status)
	{
		status = m_resize(environment,directory_result_length + 1U,RELEASE_UNUSED);
	}

	if(CRITICAL & status)
	{
		call(m_del(environment));
	}

	deliver(status);
}
