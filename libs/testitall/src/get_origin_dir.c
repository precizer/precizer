#include "testitall.h"
#include <limits.h>
#include <string.h>

// Local helper to strip trailing slashes, keeping "/" intact.
static void remove_trailing_slash_local(char *path)
{
	if(path == NULL || *path == '\0')
	{
		return;
	}

	size_t len = strlen(path);

	while(len > 1U && path[len - 1U] == '/')
	{
		path[--len] = '\0';
	}
}

/**
 * @brief Write the parent directory of the current working directory into the buffer.
 *
 * Example: if CWD is "/tmp/precizer/run", the function writes "/tmp/precizer".
 *
 * @param path Destination buffer (e.g., char path[PATH_MAX] = {0};).
 * @param path_size Size of the destination buffer in bytes (e.g., sizeof(path)).
 * @return SUCCESS on success, FAILURE on error or insufficient space.
 */
Return get_origin_dir(
	char   *path,
	size_t path_size)
{
	if(NULL == path || 0U == path_size)
	{
		return(FAILURE);
	}

	char *cwd = NULL;

#if defined(__GLIBC__)
	cwd = get_current_dir_name();
#else
	// Portable fallback for macOS/BSD.
	cwd = getcwd(NULL,0);
#endif

	if(NULL == cwd)
	{
		path[0] = '\0';
		return(FAILURE);
	}

	remove_trailing_slash_local(cwd);

	const char *last_slash = strrchr(cwd,'/');

	if(NULL == last_slash)
	{
		path[0] = '\0';
		free(cwd);
		return(FAILURE);
	}

	size_t parent_len = 0U;

	if(last_slash == cwd)
	{
		/* CWD is "/" or similar; parent is "/" */
		parent_len = 1U;
	} else {
		parent_len = (size_t)(last_slash - cwd);
	}

	const size_t needed = parent_len + 1U;

	if(needed > path_size)
	{
		path[0] = '\0';
		free(cwd);
		return(FAILURE);
	}

	if(parent_len == 1U)
	{
		path[0] = '/';
		path[1] = '\0';
	} else {
		memcpy(path,cwd,parent_len);
		path[parent_len] = '\0';
	}

	free(cwd);

	return(SUCCESS);
}
