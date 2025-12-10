#include "precizer.h"

/**
 * Save the runtime directory absolute path into global config structure,
 * fopen() was not able to process relative paths, only absolute ones.
 */
Return determine_running_dir(void)
{
	char *cwd = NULL;

#if defined(__GLIBC__)
	cwd = get_current_dir_name();
#else
	// Portable fallback for platforms without get_current_dir_name (e.g., macOS)
	cwd = getcwd(NULL, 0);
#endif

	if(cwd != NULL)
	{
		remove_trailing_slash(cwd);
		config->running_dir = cwd;
		config->running_dir_size = (long int)strlen(config->running_dir) + 1;
		slog(TRACE,"Current directory: %s\n",config->running_dir);
		provide(SUCCESS);
	} else {
		slog(ERROR,"Error getting current directory\n");
		provide(FAILURE);
	}
}
