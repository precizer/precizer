#include "precizer.h"

/**
 * @brief Returns the relative path by removing a given prefix from an absolute or relative path.
 *
 * This function checks if the given absolute path starts with the specified prefix.
 * If it does, the function returns the portion of the path after the prefix.
 * If the prefix is not found at the beginning, the original path is returned unchanged.
 *
 * Special cases:
 * - If the prefix is "/", the function removes the leading slash and returns the rest of the path.
 * - If the entire absolute path matches the prefix, it returns "." to indicate the current directory.
 * - If the absolute path does not start with the prefix, it is returned unchanged.
 *
 * @param path The absolute or relative path to be processed.
 * @param prefix The prefix to remove from the path.
 * @return A pointer to the relative path within the given absolute path.
 */
const char *extract_relative_path(
	const char *path,
	const char *prefix)
{
	size_t prefix_len = strlen(prefix);

	// Check if the prefix matches the beginning of the absolute path
	if(strncmp(path,prefix,prefix_len) == 0)
	{
		const char *relative_path = path + prefix_len;

		// Skip the leading '/' or '\' if present after the prefix
		if(*relative_path == '/' || *relative_path == '\\')
		{
			relative_path++;
		}

		// If the resulting path is empty, return "." to represent the current directory
		if(*relative_path == '\0')
		{
			return ".";
		}

		return relative_path;
	}

	// If the prefix doesn't match, return the original path
	return path;
}
