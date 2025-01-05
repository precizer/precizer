#include "precizer.h"

/**
 * @brief Checks if the file's size, creation time, and modification time have
 *        not changed since the last crawl.
 *
 * Compares data from the FTS library file traversal with the stat
 * structure stored in SQLite from the previous probe.
 *
 * @param source		Source file stat structure
 * @param destination	Destination file stat structure
 *
 * @return Return status:
 *         - IDENTICAL: Files are identical
 *         - FAILURE: Error in comparison or invalid parameters
 *         - SIZE_CHANGED
 *         - MODIFICATION_TIME_CHANGED
 *         - CREATION_TIME_CHANGED
 */
int compare_file_metadata_equivalence(
	const CmpctStat *source,
	const CmpctStat *destination
){
	/* Validate input parameters */
	if(NULL == source || NULL == destination)
	{
		return(FAILURE);
	}

	int result = IDENTICAL;

	/* Size of file, in bytes.  */
	if(source->st_size != destination->st_size)
	{
		result |= SIZE_CHANGED;

	}

	/* Modified timestamp */
	if(!(source->mtim_tv_sec == destination->mtim_tv_sec &&
	        source->mtim_tv_nsec == destination->mtim_tv_nsec))
	{
		result |= MODIFICATION_TIME_CHANGED;

	}

	/* Time of last status change  */
	if(!(source->ctim_tv_sec == destination->ctim_tv_sec &&
	        source->ctim_tv_nsec == destination->ctim_tv_nsec))
	{
		result |= CREATION_TIME_CHANGED;

	}

	return(result);
}
