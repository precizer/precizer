#include "precizer.h"
#include <errno.h>

#ifdef TESTITALL_TEST_HOOKS
static bool test_hook_path_matches_suffix(
	const char *path,
	const char *suffix)
{
	size_t path_len = 0;
	size_t suffix_len = 0;

	if(path == NULL || suffix == NULL)
	{
		return false;
	}

	if(strcmp(path,suffix) == 0)
	{
		return true;
	}

	path_len = strlen(path);
	suffix_len = strlen(suffix);

	if(path_len < suffix_len + 1U)
	{
		return false;
	}

	if(path[path_len - suffix_len - 1U] != '/')
	{
		return false;
	}

	return strcmp(path + (path_len - suffix_len),suffix) == 0;
}

static bool test_hook_override_file_access_status(
	const char       *path,
	FileAccessStatus *access_status_out)
{
	const char *target_suffix = getenv("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX");
	const char *forced_status = getenv("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS");

	if(path == NULL
	        || access_status_out == NULL
	        || target_suffix == NULL
	        || forced_status == NULL
	        || target_suffix[0] == '\0'
	        || forced_status[0] == '\0')
	{
		return false;
	}

	if(test_hook_path_matches_suffix(path,target_suffix) == false)
	{
		return false;
	}

	if(strcmp(forced_status,"FILE_ACCESS_ALLOWED") == 0)
	{
		*access_status_out = FILE_ACCESS_ALLOWED;

	} else if(strcmp(forced_status,"FILE_ACCESS_DENIED") == 0){
		*access_status_out = FILE_ACCESS_DENIED;

	} else if(strcmp(forced_status,"FILE_NOT_FOUND") == 0){
		*access_status_out = FILE_NOT_FOUND;

	} else if(strcmp(forced_status,"FILE_ACCESS_ERROR") == 0){
		*access_status_out = FILE_ACCESS_ERROR;

	} else {
		slog(ERROR,"Test hook failed: unsupported TESTITALL_TEST_ENV_FILE_ACCESS_STATUS value %s\n",forced_status);
		*access_status_out = FILE_ACCESS_ERROR;
	}

	return true;
}
#endif

/**
 * @brief Classify errno from a failed access() into FileAccessStatus
 *
 * Maps common filesystem errors to a stable, high-level status:
 * - ENOENT, ENOTDIR -> FILE_NOT_FOUND
 * - EACCES, EPERM   -> FILE_ACCESS_DENIED
 * - otherwise       -> FILE_ACCESS_ERROR
 *
 * @param err errno value (typically `errno` after a failed `access()` call)
 * @return FileAccessStatus classification
 *
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
 * @brief Check access for a path, first as provided, then by its absolute form
 *
 * @details
 * The function first calls `access()` for the path exactly as supplied
 * If that succeeds, it returns `FILE_ACCESS_ALLOWED`
 *
 * If the initial call fails, the function then tries to construct an
 * absolute path from `config->running_dir`
 * When that fallback path can be built, the result is determined by the
 * second `access()` call for the constructed absolute path
 * If the fallback path cannot be built, the function returns
 * `FILE_ACCESS_ERROR`
 *
 * @param path Path to check, relative or absolute
 * @param path_size Length of the provided path
 * @param mode Access mode for `access()` such as `R_OK` or `X_OK`
 * @return `FILE_ACCESS_ALLOWED`, `FILE_ACCESS_DENIED`, `FILE_NOT_FOUND`, or
 *         `FILE_ACCESS_ERROR` based on the direct check or the fallback
 *         absolute-path check
 */
FileAccessStatus file_check_access(
	const char   *path,
	const size_t path_size,
	const int    mode)
{
#ifdef TESTITALL_TEST_HOOKS
	FileAccessStatus forced_status = FILE_ACCESS_ALLOWED;

	if(test_hook_override_file_access_status(path,&forced_status) == true)
	{
		return(forced_status);
	}
#endif

	if(access(path,mode) == 0)
	{
		return(FILE_ACCESS_ALLOWED);
	}

	FileAccessStatus access_status;

	char *absolute_path = NULL;

	if(TRIUMPH & path_absolute_from_relative(&absolute_path,path,path_size))
	{
		if(access(absolute_path,mode) == 0)
		{
			access_status = FILE_ACCESS_ALLOWED;

		} else {

			access_status = classify_access_errno(errno);
		}
	} else {
		access_status = FILE_ACCESS_ERROR;
	}

	if(absolute_path != NULL)
	{
		free(absolute_path);
	}

	return(access_status);
}
