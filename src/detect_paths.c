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
	const char *path,
	const unsigned char fs_object_type
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Do nothing if the path is not a real path but in-memory database
	if(strcmp(path,":memory:") == 0)
	{
		return(status);
	}

	struct stat stats;

	slog(TRACE,"Verify that the path %s exists\n",path);

	// Check for existence
	if(stat(path, &stats) == 0)
	{
		// Check is it a directory or a file
		if(fs_object_type == SHOULD_BE_A_FILE)
		{
			if(S_ISREG(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a file\n",path);

				status = SUCCESS;
			} else {
				status = FAILURE;
			}

		} else {
			if(S_ISDIR(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a directory\n",path);

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
		if(fs_object_type == SHOULD_BE_A_FILE)
		{
			slog(EVERY,"The path %s doesn't exist or it is not a file\n",path);
		} else {
			slog(EVERY,"The path %s doesn't exist or it is not a directory\n",path);
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
		// The option to compare databases has been selected.
		// There is no need to compare paths
		slog(TRACE,"Comparing databases. Directory path verification is not required\n");
		return(status);
	} else {
		// Check directory paths passed as arguments, traverse
		// them for files, and store the file metadata in the database
		slog(TRACE,"Checking directory paths provided as arguments\n");
	}

	for (int i = 0; config->paths[i]; i++)
	{
		if(SUCCESS != (status = detect_a_path(config->paths[i],SHOULD_BE_A_DIRECTORY)))
		{
			// The path doesn't exist or is not a directory
			return(status);
		}
	}

	slog(TRACE,"Paths detected\n");

	return(status);
}
