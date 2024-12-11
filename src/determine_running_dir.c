#include "precizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * Save the runtime directory absolute path into global config structure,
 * fopen() was not able to process relative paths, only absolute ones.
 */
Return determine_running_dir(void)
{
	char *cwd = get_current_dir_name();
	if (cwd != NULL) {
		config->running_dir = cwd;
		config->running_dir_size = (long int)strlen(config->running_dir) + 1;
		slog(TRACE,"Current directory: %s\n", config->running_dir);
		return(SUCCESS);
	} else {
		slog(ERROR, "Error getting current directory\n");
		return(FAILURE);
	}
}
