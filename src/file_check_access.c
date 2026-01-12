#include "precizer.h"
#include <errno.h>

/**
 * @brief Checks if a file is accessible for reading.
 *
 * This function verifies whether the specified file is readable.
 * If the direct access check fails, it attempts to resolve the absolute path
 * and checks again.
 *
 * @param[in] path Pointer to the file path string.
 * @param[in] path_size Pointer to the length of the path string.
 *
 * @return FileAccessStatus indicating accessibility, denial, or error.
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
 * Check read access for a path, first as provided, then by its absolute form.
 *
 * The function preserves the errno classification from the initial `access`
 * call and only overwrites it if resolving the absolute path succeeds and a
 * second `access` call provides a more precise status.
 *
 * @param path       Path to check (relative or absolute).
 * @param path_size  Length of the provided path.
 * @return FILE_ACCESS_ALLOWED on success; FILE_NOT_FOUND, FILE_ACCESS_DENIED,
 *         or FILE_ACCESS_ERROR depending on errno classification.
 */
FileAccessStatus file_check_access(
	const char   *path,
	const size_t path_size)
{
	if(access(path,R_OK) == 0)
	{
		return(FILE_ACCESS_ALLOWED);
	}

	FileAccessStatus access_status = classify_access_errno(errno);

	char *absolute_path = NULL;

	if(SUCCESS == path_absolute_from_relative(&absolute_path,path,path_size))
	{
		if(access(absolute_path,R_OK) == 0)
		{
			access_status = FILE_ACCESS_ALLOWED;

		} else {

			access_status = classify_access_errno(errno);
		}
	}

	free(absolute_path);

	return(access_status);
}
