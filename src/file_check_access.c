#include "precizer.h"
#include <errno.h>

/**
 * @brief Classify errno from a failed access() into FileAccessStatus
 *
 * Maps common filesystem errors to a stable, high-level status:
 * - ENOENT, ENOTDIR -> FILE_NOT_FOUND
 * - EACCES, EPERM   -> FILE_ACCESS_DENIED
 * - otherwise       -> FILE_ACCESS_ERROR
 *
 * @param err errno value (typically `errno` after a failed `access()` call)
 * @return FileAccessStatus classification
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
 * @brief Check access for a path, first as provided, then by its absolute form
 *
 * @details
 * The function first calls `access()` for the path exactly as supplied
 * If that succeeds, it returns `FILE_ACCESS_ALLOWED`
 *
 * If the initial call fails, the function then tries to construct an
 * absolute path from `config->running_dir`
 * When that fallback path can be built, the result is determined by the
 * second `access()` call for the constructed absolute path
 * If the fallback path cannot be built, the function returns
 * `FILE_ACCESS_ERROR`
 *
 * @param path Path to check, relative or absolute
 * @param path_size Length of the provided path
 * @param mode Access mode for `access()` such as `R_OK` or `X_OK`
 * @return `FILE_ACCESS_ALLOWED`, `FILE_ACCESS_DENIED`, `FILE_NOT_FOUND`, or
 *         `FILE_ACCESS_ERROR` based on the direct check or the fallback
 *         absolute-path check
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

	FileAccessStatus access_status;

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
