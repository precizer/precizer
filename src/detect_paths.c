#include "precizer.h"
#include <unistd.h>
#include <sys/stat.h>

/**
 *
 * Function to check whether a directory exists or not.
 * @returns Returns SUCCESS if given path is directory and exists
 *                  FAILURE otherwise
 *
 */
Return detect_a_path(
	const char *path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	struct stat stats;

	slog(true,"Check up the path %s availability\n",path);

	// Check for existence
	if(stat(path, &stats) == 0)
	{
		// Check is it a directory or a file
		if(config->compare == true)
		{
			if(S_ISREG(stats.st_mode))
			{
				slog(true,"The path %s is exists and it is a file\n",path);
				status = SUCCESS;

			} else {
				status = FAILURE;
			}

		} else {
			if(S_ISDIR(stats.st_mode))
			{
				slog(true,"The path %s is exists and it is a directory\n",path);
				status = SUCCESS;
			} else {
				status = FAILURE;
			}
		}

	} else {
		status = FAILURE;
	}

	if(SUCCESS != status)
	{
		if(config->compare == true)
		{
			slog(false,"The path %s doesn't exist or it is not a database file\n",path);
		} else {
			slog(false,"The path %s doesn't exist or it is not a directory\n",path);
		}
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

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

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
