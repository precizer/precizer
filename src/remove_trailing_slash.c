#include "precizer.h"

/**
 * @brief Removes trailing slashes from a given path.
 *
 * Modifies the input string in-place by removing any trailing '/' characters.
 * If the path consists only of slashes, it reduces it to a single '/'.
 *
 * @param path The string representing the path to be modified.
 */
void remove_trailing_slash(char *path)
{
	if(path == NULL || *path == '\0')
	{
		return;
	}

	size_t len = strlen(path);

	// Avoid modifying "/" (root path)
	while(len > 1 && path[len - 1] == '/')
	{
		path[--len] = '\0';
	}
}
