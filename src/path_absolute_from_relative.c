#include "precizer.h"

/**
 * @brief Allocate a path string suitable for filesystem access
 *
 * @details When `path` is relative, the function prefixes it with
 * `config->running_dir`. When `path` is already absolute, the function copies it
 * as-is into a newly allocated buffer
 *
 * @param[out] absolute_path Pointer that receives the allocated path buffer
 * @param[in] path Relative or absolute input path
 * @param[in] path_size Length of `path` without the terminating null byte
 * @return Return status code
 */
Return path_absolute_from_relative(
	char         **absolute_path,
	const char   *path,
	const size_t path_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	size_t len = 0;

	if(!absolute_path || !path)
	{
		provide(FAILURE);
	}

	// Allocate memory for the absolute path (base dir + '/' + relative path + null terminator)
	if(path_size > 0 && path[0] == '/')
	{
		// The provided path is actually absolute!
		len = path_size + 1;
		*absolute_path = (char *)malloc(len);

		if(*absolute_path == NULL)
		{
			report("Memory allocation failed, requested size: %zu bytes",len);
			status = FAILURE;
			provide(status);
		}

		snprintf(*absolute_path,len,"%s",path);

	} else {
		// The provided path is indeed relative!
		// running_dir_size already counts the trailing '\0'; +1 adds space for '/' and the new terminator.
		len = (size_t)config->running_dir_size + path_size + 1;
		*absolute_path = (char *)malloc(len);

		if(*absolute_path == NULL)
		{
			report("Memory allocation failed, requested size: %zu bytes",len);
			status = FAILURE;
			provide(status);
		}

		snprintf(*absolute_path,len,"%s/%s",config->running_dir,path);
	}

	provide(status);
}
