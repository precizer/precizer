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
		return(FILE_ACCESS_NOT_FOUND);
	}

	if(err == EACCES || err == EPERM)
	{
		return(FILE_ACCESS_DENIED);
	}

	return(FILE_ACCESS_ERROR);
}

FileAccessStatus file_check_access(
	const char   *path,
	const size_t path_size)
{

	if(access(path,R_OK) == 0)
	{
		return(FILE_ACCESS_ALLOWED);

	} else {
		int first_err = errno;

		char *absolute_path = NULL;

		Return status = path_absolute_from_relative(&absolute_path,path,path_size);

		FileAccessStatus access_status = FILE_ACCESS_ERROR;

		if(SUCCESS == status)
		{
			if(access(absolute_path,R_OK) == 0)
			{
				access_status = FILE_ACCESS_ALLOWED;
			} else {
				access_status = classify_access_errno(errno);
			}
		} else {
			access_status = FILE_ACCESS_ERROR;
		}

		free(absolute_path);
		if(access_status != FILE_ACCESS_ERROR)
		{
			return(access_status);
		}

		// Fallback to classification from initial relative check if absolute path failed to resolve
		return(classify_access_errno(first_err));
	}
}
