#include "precizer.h"
#include <unistd.h>
#include <sys/stat.h>

/**
 * Function to check whether a directory exists or not.
 * It returns SUCCESS if given path is directory and exists 
 * otherwise returns FAILURE.
 */
Return detect_a_path(
	const char *path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	struct stat stats;

	// Check for existence
	if(stat(path, &stats) == 0)
	{
		// Check is it a directory or not
		if(!S_ISDIR(stats.st_mode))
		{
			status = FAILURE;
		}
	} else {
		status = FAILURE;
	}

	if(SUCCESS != status)
	{
		slog(false,"The path %s doesn't exist or it is not a directory\n",path);
	}

	return(status);
}

/**
 *
 * Check all paths passed as arguments.
 * Are they directories and do they exist?
 *
 */

Return detect_paths(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	for (int i = 0; config->paths[i]; i++)
	{
		if(SUCCESS != (status = detect_a_path(config->paths[i])))
		{
			// The path doesn't exist or is not a directory
			return(status);
		}
	}

	return(status);
}
