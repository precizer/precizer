#include "precizer.h"

/**
 *
 * Check all paths passed as arguments.
 * Are they directories and do they exist?
 *
 */

Return paths_detect(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		// The option to compare databases has been selected.
		// There is no need to compare paths
		slog(TRACE,"Comparing databases. Directory path verification is not required\n");
		provide(status);
	} else {
		// Check directory paths passed as arguments, traverse
		// them for files, and store the file metadata in the database
		slog(TRACE,"Checking directory paths provided as arguments\n");
	}

	if(config->paths == NULL)
	{
		slog(ERROR,"The PATH is not defined\n");
		status = FAILURE;
	}

	for(int i = 0; config->paths[i]; i++)
	{
		if(SUCCESS == status && NOT_FOUND == file_availability(config->paths[i],SHOULD_BE_A_DIRECTORY))
		{
			status = FAILURE;
		}
	}

	slog(TRACE,"Path detection is finished\n");

	provide(status);
}
