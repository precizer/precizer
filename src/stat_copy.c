#include "precizer.h"

/**
 *
 *
 */
Return stat_copy(
	const struct stat *source,
	CmpctStat         *destination
){
	Return status = SUCCESS;

	if(source == NULL || destination == NULL)
	{
		return(FAILURE);
	}

	/* Copying essential elements from the stat structure to the new one */
	destination->st_size      = source->st_size;
	destination->mtim_tv_sec  = source->st_mtim.tv_sec;
	destination->mtim_tv_nsec = source->st_mtim.tv_nsec;
	destination->ctim_tv_sec  = source->st_ctim.tv_sec;
	destination->ctim_tv_nsec = source->st_ctim.tv_nsec;

	return(status);
}
