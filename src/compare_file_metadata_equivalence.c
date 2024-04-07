#include "precizer.h"

/**
 *
 * The function check up if size, creation and modification time of a file did
 * not change since last crawling.
 * It takes data from FTS library traversing of the file and compare with the
 * structure "stat" stored against SQLite after previous probe
 *
 */
int compare_file_metadata_equivalence
(
	const struct stat *source,
	const struct stat *destination
){
	int result = IDENTICAL;

	/* Size of file, in bytes.  */
	if(source->st_size != destination->st_size)
	{
		result += SIZE_CHANGED;

	}

	/* Modified timestamp */
	if(!(source->st_mtim.tv_sec == destination->st_mtim.tv_sec &&
			source->st_mtim.tv_nsec == destination->st_mtim.tv_nsec))
	{
		result += MODIFICATION_TIME_CHANGED;

	}

	/* Time of last status change  */
	if(!(source->st_ctim.tv_sec == destination->st_ctim.tv_sec &&
			source->st_ctim.tv_nsec == destination->st_ctim.tv_nsec))
	{
		result += CREATION_TIME_CHANGED;

	}

	return(result);
}
