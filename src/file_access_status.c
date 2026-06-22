#include "precizer.h"
#include <errno.h>

/**
 * @brief Classify a filesystem access errno value into FileAccessStatus
 *
 * Maps common filesystem errors to a stable, high-level status:
 * - ENOENT, ENOTDIR -> FILE_NOT_FOUND
 * - EACCES, EPERM   -> FILE_ACCESS_DENIED
 * - otherwise       -> FILE_ACCESS_ERROR
 *
 * @param err errno value from a failed filesystem path operation
 * @return FileAccessStatus classification
 */
FileAccessStatus file_access_status(const int err)
{
	if(err == ENOENT || err == ENOTDIR)
	{
		return(FILE_NOT_FOUND);
	}

	if(err == EACCES || err == EPERM)
	{
		return(FILE_ACCESS_DENIED);
	}

	return(FILE_ACCESS_ERROR);
}
