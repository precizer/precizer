#include "precizer.h"
#include <errno.h>
#include <fcntl.h>

/**
 * @brief Open a directory as the base for relative path access checks
 *
 * @details
 * Uses the platform search-only directory mode when available. macOS and
 * Cygwin provide `O_SEARCH`, while Linux provides `O_PATH`. The fallback uses
 * a read-only directory descriptor for platforms without either extension
 *
 * `O_SEARCH` and `O_PATH` allow the descriptor to serve as a path-resolution
 * base without requiring directory enumeration. The descriptor is opened with
 * close-on-exec and is verified to refer to a directory
 *
 * Test builds can force an unavailable result for a matching path. The test
 * override also sets a representative `errno` value so callers exercise the
 * same diagnostics as a real `open()` failure
 *
 * @param[in] directory_path Descriptor containing the directory path to open
 * @param[out] directory_fd_out Receives the opened descriptor. Set to -1 before
 *             `open()` is attempted
 * @return `FILE_ACCESS_ALLOWED` when the directory was opened.
 *         `FILE_NOT_FOUND` or `FILE_ACCESS_DENIED` for the corresponding
 *         `open()` failure. `FILE_ACCESS_ERROR` when the output pointer is
 *         invalid or another `open()` failure cannot be classified more
 *         specifically
 */
FileAccessStatus directory_open(
	const memory *directory_path,
	int          *directory_fd_out)
{
	if(directory_fd_out == NULL)
	{
		errno = EINVAL;
		return(FILE_ACCESS_ERROR);
	}

	*directory_fd_out = -1;

	const char *runtime_directory_path = m_text(directory_path);

#ifdef TESTITALL_TEST_HOOKS
	FileAccessStatus forced_status = FILE_ACCESS_ALLOWED;

	if(testitall_file_access_status_override(runtime_directory_path,&forced_status) == true
	        && forced_status != FILE_ACCESS_ALLOWED)
	{
		if(forced_status == FILE_ACCESS_DENIED)
		{
			errno = EACCES;

		} else if(forced_status == FILE_NOT_FOUND){
			errno = ENOENT;

		} else {
			errno = EIO;
		}

		return(forced_status);
	}
#endif

	int open_flags = O_DIRECTORY | O_CLOEXEC;

	/*
	 * Open the directory with the least access required to use it as the starting
	 * point for relative paths:
	 *
	 * - O_SEARCH is the POSIX-style option for opening a directory so known child
	 *   names can be resolved without asking permission to list its contents.
	 * - O_PATH is the Linux alternative. It creates a lightweight descriptor that
	 *   refers to the directory and can be used by functions such as faccessat().
	 * - O_RDONLY is the portable fallback when neither specialized option exists.
	 *   It may require directory read permission even though this function does
	 *   not need to enumerate the directory
	 */
#if defined(O_SEARCH)
	open_flags |= O_SEARCH;
#elif defined(O_PATH)
	open_flags |= O_PATH;
#else
	open_flags |= O_RDONLY;
#endif

	const int directory_fd = open(runtime_directory_path,open_flags);

	if(directory_fd >= 0)
	{
		*directory_fd_out = directory_fd;
		return(FILE_ACCESS_ALLOWED);
	}

	return(file_access_status(errno));
}
