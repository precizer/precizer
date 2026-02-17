#include "precizer.h"
#include <errno.h>

/**
 * @brief Classify errno from a failed access() into FileAccessStatus.
 *
 * Maps common filesystem errors to a stable, high-level status:
 * - ENOENT, ENOTDIR -> FILE_NOT_FOUND
 * - EACCES, EPERM   -> FILE_ACCESS_DENIED
 * - otherwise       -> FILE_ACCESS_ERROR
 *
 * @param err errno value (typically `errno` after a failed `access()` call).
 * @return FileAccessStatus classification.
 *
 */
static FileAccessStatus classify_access_errno(int err)
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

/**
 * Check access for a path, first as provided, then by its absolute form.
 *
 * The function preserves the errno classification from the initial `access`
 * call and only overwrites it if resolving the absolute path succeeds and a
 * second `access` call provides a more precise status.
 *
 * @param path       Path to check (relative or absolute).
 * @param path_size  Length of the provided path.
 * @param mode       Access mode for `access()` (e.g., R_OK, X_OK).
 * @return FILE_ACCESS_ALLOWED on success; FILE_NOT_FOUND, FILE_ACCESS_DENIED,
 *         or FILE_ACCESS_ERROR depending on errno classification.
 */
FileAccessStatus file_check_access(
	const char   *path,
	const size_t path_size,
	const int    mode)
{
	if(access(path,mode) == 0)
	{
		return(FILE_ACCESS_ALLOWED);
	}

	FileAccessStatus access_status = classify_access_errno(errno);

	char *absolute_path = NULL;

	if(TRIUMPH & path_absolute_from_relative(&absolute_path,path,path_size))
	{
		if(access(absolute_path,mode) == 0)
		{
			access_status = FILE_ACCESS_ALLOWED;

		} else {

			access_status = classify_access_errno(errno);
		}
	} else {
		access_status = FILE_ACCESS_ERROR;
	}

	if(absolute_path != NULL)
	{
		free(absolute_path);
	}

	return(access_status);
}
