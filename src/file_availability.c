#include "precizer.h"
#include <unistd.h>
#include <sys/stat.h>

/**
 *
 * Function to check whether a directory exists or not.
 * @returns Returns EXISTS if given path is directory and exists
 *                  NOT_FOUND otherwise
 *
 */
FileAvailability file_availability(
	const char          *path,
	const unsigned char fs_object_type
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
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
			if(S_ISREG(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a file\n",path);

				presence = EXISTS;
			} else {
				presence = NOT_FOUND;
			}

		} else {
			if(S_ISDIR(stats.st_mode))
			{
				slog(TRACE,"The path %s is exists and it is a directory\n",path);

				presence = EXISTS;
			} else {
				presence = NOT_FOUND;
			}
		}

	} else {
		presence = NOT_FOUND;
	}

	if(EXISTS != presence)
	{
		if(fs_object_type == SHOULD_BE_A_FILE)
		{
			slog(EVERY,"The path %s doesn't exist or it is not a file\n",path);
		} else {
			slog(EVERY,"The path %s doesn't exist or it is not a directory\n",path);
		}
	}

	return(presence);
}
