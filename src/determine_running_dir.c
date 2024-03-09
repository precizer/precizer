#include "precizer.h"
#include <unistd.h>
#include <errno.h>

/**
 * Remember the runtime directory absolute path in config,
 * fopen() was not able to process relative paths, only absolute ones.
 */
Return determine_running_dir(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *ptr;

	config->running_dir_size = pathconf(".", _PC_PATH_MAX);
	if (config->running_dir_size == -1){
		config->running_dir_size = 1024;
	} else if (config->running_dir_size > 10240){
		config->running_dir_size = 10240;
	}

	for(config->running_dir = ptr = NULL; ptr == NULL; config->running_dir_size *= 2)
	{
		char *tmp = (char *)realloc(config->running_dir,(size_t)config->running_dir_size);
		if(NULL == tmp)
		{
			slog(false,"Realloc error\n");
			free(config->running_dir);
			status = FAILURE;
			break;
		} else {
			config->running_dir = tmp;
		}

		ptr = getcwd(config->running_dir,(size_t)config->running_dir_size);
		if (ptr == NULL && errno != ERANGE)
		{
			slog(false,"ERANGE error\n");
			status = FAILURE;
		}
	}

	config->running_dir_size = (long int)strlen(config->running_dir) + 1;

	// Reduce running_dir size to the real one
	char *tmp = (char *)realloc(config->running_dir,(size_t)config->running_dir_size);
	if(NULL == tmp)
	{
		slog(false,"Realloc error\n");
		free(config->running_dir);
		status = FAILURE;
	} else {
		config->running_dir = tmp;
	}

	return(status);
}
