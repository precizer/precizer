#include "precizer.h"

/**
 * @brief Check whether the given path exists and matches requested filesystem object type
 * @param path Path to verify
 * @param fs_object_type Expected object type: SHOULD_BE_A_FILE or SHOULD_BE_A_DIRECTORY
 * @param output_stat Optional pointer to a stat structure to receive file metadata when the
 *        path is found. Pass NULL if metadata is not needed
 * @return EXISTS when path exists and matches requested type, otherwise NOT_FOUND
 * @details The special path ":memory:" is treated as not available and returns NOT_FOUND
 */
FileAvailability file_availability(
	const char          *path,
	struct stat         *output_stat,
	const unsigned char fs_object_type)
{
	/// The availability status returned by this function
	/// By default, the path is treated as unavailable until verified
	FileAvailability presence = NOT_FOUND;

	// Do nothing if the path is not a real path but in-memory database
	if(strcmp(path,":memory:") == 0)
	{
		return(presence);
	}

	struct stat stats;

	slog(TRACE,"Verify that the path %s exists\n",path);

	// Check for existence
	if(stat(path,&stats) == 0)
	{
		// Check is it a directory or a file
		if(fs_object_type == SHOULD_BE_A_FILE)
		{
			// Verify that the path points to a regular file
			if(S_ISREG(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a file\n",path);
				presence = EXISTS;
			}

		} else if(fs_object_type == SHOULD_BE_A_DIRECTORY){
			// Verify that the path points to a directory
			if(S_ISDIR(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a directory\n",path);
				presence = EXISTS;
			}
		}

		// Copy metadata to caller-provided buffer if the path was found
		if(EXISTS == presence && output_stat != NULL)
		{
			*output_stat = stats;
		}
	}

	if(EXISTS != presence)
	{
		if(fs_object_type == SHOULD_BE_A_FILE)
		{
			slog(EVERY,"The path %s doesn't exist or it is not a file\n",path);
		} else if(fs_object_type == SHOULD_BE_A_DIRECTORY){
			slog(EVERY,"The path %s doesn't exist or it is not a directory\n",path);
		}
	}

	return(presence);
}
