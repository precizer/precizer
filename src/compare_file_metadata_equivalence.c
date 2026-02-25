#include "precizer.h"

/**
 * @brief Checks whether key file metadata changed since the last crawl.
 *
 * Compares data from the FTS library file traversal with the stat
 * structure stored in SQLite from the previous probe.
 *
 * @param source		Source file stat structure
 * @param destination	Destination file stat structure
 *
 * @return Return status:
 *         - IDENTICAL: Files are identical
 *         - COMPARE_FAILED: Error in comparison or invalid parameters
 *         - SIZE_CHANGED (logical file size changed)
 *         - ALLOCATED_SIZE_CHANGED (allocated block count changed)
 *         - MODIFICATION_TIME_CHANGED
 *         - STATUS_CHANGED_TIME
 */
Changed compare_file_metadata_equivalence(
	const CmpctStat *source,
	const CmpctStat *destination)
{
	/* Validate input parameters */
	if(NULL == source || NULL == destination)
	{
		return(COMPARE_FAILED);
	}

	Changed changes = IDENTICAL;

	/* Logical file size in bytes. */
	if(source->st_size != destination->st_size)
	{
		changes |= SIZE_CHANGED;
	}

	/* Allocated size of file in POSIX 512-byte blocks. */
#if 0
	if(source->st_blocks != destination->st_blocks)
#else
	/* This legacy can be removed in 2036 (10-year Long-Term Support) */
	if(source->st_blocks != BLKCNT_UNKNOWN
	        && source->st_blocks != destination->st_blocks)
#endif
	{
		changes |= ALLOCATED_SIZE_CHANGED;
	}

	/* Modified timestamp */
	if(!(source->mtim_tv_sec == destination->mtim_tv_sec &&
	        source->mtim_tv_nsec == destination->mtim_tv_nsec))
	{
		changes |= MODIFICATION_TIME_CHANGED;
	}

	/* Time of last status change  */
	if(!(source->ctim_tv_sec == destination->ctim_tv_sec &&
	        source->ctim_tv_nsec == destination->ctim_tv_nsec))
	{
		changes |= STATUS_CHANGED_TIME;
	}

	return(changes);
}
