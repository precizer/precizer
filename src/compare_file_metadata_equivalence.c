#include "precizer.h"

/**
 * The function check up if size, creation and modification time of a file did
 * not change since last scanning.
 * It takes data from FTS library traversing of the file and compare with the
 * structure "stat" stored against SQLite after previous probe
 */
int compare_file_metadata_equivalence
(
	const struct stat *source,
	const struct stat *destination
){
	int result = IDENTICAL;

	if(source->st_size != destination->st_size) /* Size of file, in bytes.  */
	{
		result += SIZE_CHANGED;

	}

	if(!(source->st_mtim.tv_sec == destination->st_mtim.tv_sec &&
			source->st_mtim.tv_nsec == destination->st_mtim.tv_nsec)) /* Modified timestamp */
	{
		result += MODIFICATION_TIME_CHANGED;

	}

	if(!(source->st_ctim.tv_sec == destination->st_ctim.tv_sec &&
			source->st_ctim.tv_nsec == destination->st_ctim.tv_nsec)) /* Time of last status change  */
	{
		result += CREATION_TIME_CHANGED;

	}

	return(result);
}
