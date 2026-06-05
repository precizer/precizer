#include "precizer.h"

/**
 * @brief Store the current working directory as a normalized absolute path
 *
 * Saves the process working directory into the global configuration as a
 * managed string descriptor. This path is later used as the base directory
 * when absolute paths must be constructed from relative inputs
 *
 * @return SUCCESS when the working directory was captured and normalized,
 *         otherwise FAILURE
 */
Return determine_running_dir(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

#if defined(__GLIBC__)
	char *cwd = get_current_dir_name();
#else
	// Portable fallback for platforms without get_current_dir_name (e.g., macOS)
	char *cwd = getcwd(NULL,0);
#endif

	if(cwd == NULL)
	{
		slog(ERROR,"Error getting current directory\n");
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		run(m_copy_string(conf(running_dir),cwd));
	}

	if(cwd != NULL)
	{
		free(cwd);
	}

	if(TRIUMPH & status)
	{
		run(remove_trailing_slash(conf(running_dir)));
	}

	if(TRIUMPH & status)
	{
		slog(TRACE,"Current directory: %s\n",confstr(running_dir));
	}

	provide(status);
}
