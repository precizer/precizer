#include "precizer.h"
#include <errno.h>

/**
 * @brief Check access with an absolute-path fallback
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
 *
 * @deprecated New callers should use file_check_access() with an explicitly
 *             opened directory descriptor
 */
FileAccessStatus file_check_access_absolute(
	const char   *path,
	const size_t path_size,
	const int    mode)
{
	// TODO: Replace remaining uses of this legacy implementation with the directory-relative file_check_access()
#ifdef TESTITALL_TEST_HOOKS
	FileAccessStatus forced_status = FILE_ACCESS_ALLOWED;

	if(testitall_file_access_status_override(path,&forced_status) == true)
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

			access_status = file_access_status(errno);
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

/**
 * @brief Check access to a path using an open directory as its base
 *
 * @details
 * Passes the supplied path to `faccessat()` without changing the process
 * working directory or constructing an absolute path. A relative path is
 * resolved from @p directory_fd. As specified by `faccessat()`, an absolute
 * path is resolved independently and causes @p directory_fd to be ignored
 *
 * The function uses the process real user and group IDs, matching `access()`
 * semantics. A successful check returns `FILE_ACCESS_ALLOWED`; a failed check
 * is translated from `errno` into the corresponding `FileAccessStatus`
 *
 * @param[in] directory_fd File descriptor used as the base for a relative path
 * @param[in] relative_path Path passed to `faccessat()`. Callers that require
 *            root confinement must ensure that this value is relative
 * @param[in] mode Access mode such as `F_OK`, `R_OK`, `W_OK`, or `X_OK`
 * @return `FILE_ACCESS_ALLOWED`, `FILE_ACCESS_DENIED`, `FILE_NOT_FOUND`, or
 *         `FILE_ACCESS_ERROR`. A NULL path returns `FILE_ACCESS_ERROR`
 */
FileAccessStatus file_check_access(
	const int  directory_fd,
	const char *relative_path,
	const int  mode)
{
	if(relative_path == NULL)
	{
		return(FILE_ACCESS_ERROR);
	}

#ifdef TESTITALL_TEST_HOOKS
	FileAccessStatus forced_status = FILE_ACCESS_ALLOWED;

	if(testitall_file_access_status_override(relative_path,&forced_status) == true)
	{
		return(forced_status);
	}
#endif

	if(faccessat(directory_fd,relative_path,mode,0) == 0)
	{
		return(FILE_ACCESS_ALLOWED);
	}

	const int access_errno = errno;

	return(file_access_status(access_errno));
}
