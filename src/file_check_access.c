#include "precizer.h"

/**
 * @brief Checks if a file is accessible for reading.
 *
 * This function verifies whether the specified file is readable.
 * If the direct access check fails, it attempts to resolve the absolute path
 * and checks again.
 *
 * @param[in] path Pointer to the file path string.
 * @param[in] path_size Pointer to the length of the path string.
 * @param[out] is_readable Pointer to a boolean variable that will be set to:
 *              - true if the file is readable,
 *              - false if the file is not readable.
 *
 * @return SUCCESS if function executed correctly, otherwise an error code.
 */
Return file_check_access(
	const char               *path,
	const short unsigned int *path_size,
	bool                     *is_readable)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(access(path,R_OK) == 0)
	{
		*is_readable = true;

	} else {
		char *absolute_path = NULL;
		status = path_absolute_from_relative(&absolute_path,path,path_size);

		if(SUCCESS == status)
		{
			if(access(absolute_path,R_OK) == 0)
			{
				*is_readable = true;
			} else {
				*is_readable = false;
			}
		}
		free(absolute_path);
	}

	provide(status);
}
