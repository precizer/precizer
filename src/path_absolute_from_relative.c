#include "precizer.h"

/**
 * @brief Constructs an absolute path from a base directory and a relative path.
 *
 * This function dynamically allocates memory for the absolute path and combines
 * the given base directory (config->running_dir) with the relative path.
 *
 * @param absolute_path Pointer to store the dynamically allocated absolute path.
 * @param path Relative path to append.
 * @param path_size Size of the relative path string.
 */
Return path_absolute_from_relative(
	char                     **absolute_path,
	const char               *path,
	const short unsigned int *path_size)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	size_t len = 0;

	if(!absolute_path || !path)
	{
		provide(FAILURE);
	}

	// Allocate memory for the absolute path (base dir + '/' + relative path + null terminator)
	if(*path_size > 0 && path[0] == '/')
	{
		// The provided path is actually absolute!
		len = (size_t)*path_size + 1;
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
		len = (size_t)config->running_dir_size + (size_t)*path_size + 1;
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
