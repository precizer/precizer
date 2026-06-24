#include "precizer.h"
#include <errno.h>

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
 * @param[in] relative_path String memory descriptor whose text is passed to
 *            `faccessat()`. Callers that require root confinement must ensure
 *            that this value is relative
 * @param[in] mode Access mode such as `F_OK`, `R_OK`, `W_OK`, or `X_OK`
 * @return `FILE_ACCESS_ALLOWED`, `FILE_ACCESS_DENIED`, `FILE_NOT_FOUND`, or
 *         `FILE_ACCESS_ERROR`. A NULL path descriptor returns
 *         `FILE_ACCESS_ERROR`
 */
FileAccessStatus file_check_access(
	const int    directory_fd,
	const memory *relative_path,
	const int    mode)
{
	if(relative_path == NULL)
	{
		return(FILE_ACCESS_ERROR);
	}

	const char *runtime_relative_path = m_text(relative_path);

#ifdef TESTITALL_TEST_HOOKS
	FileAccessStatus forced_status = FILE_ACCESS_ALLOWED;

	if(testitall_file_access_status_override(runtime_relative_path,&forced_status) == true)
	{
		return(forced_status);
	}
#endif

	if(faccessat(directory_fd,runtime_relative_path,mode,0) == 0)
	{
		return(FILE_ACCESS_ALLOWED);
	}

	return(file_access_status(errno));
}
