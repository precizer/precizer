#include "testitall.h"
#include <errno.h>
#include <ftw.h>

/**
 * @brief Remove callback used by nftw
 *
 * @param[in] path Current path from nftw walk
 * @param[in] stat_buffer Unused file stat pointer from nftw
 * @param[in] type_flag Unused type flag from nftw
 * @param[in] ftw_buffer Unused nftw frame data
 *
 * @return 0 on success, non-zero on failure
 */
static int nftw_remove_callback(
	const char        *path,
	const struct stat *stat_buffer,
	const int         type_flag,
	struct FTW        *ftw_buffer)
{
	(void)stat_buffer;
	(void)type_flag;
	(void)ftw_buffer;

	if(remove(path) != 0)
	{
		echo(STDERR,"delete_path: remove failed for %s: %s\n",path,strerror(errno));
		return(-1);
	}

	return(0);
}

/**
 * @brief Remove file or directory tree by path relative to TMPDIR
 *
 * When relative_path_to_tmpdir is an empty string, the function targets TMPDIR itself.
 * Missing paths are treated as a hard failure to keep test cleanup strict and deterministic
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Path was removed
 *         - FAILURE: Path construction or remove traversal failed
 */
Return delete_path(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat path_stat = {0};
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		echo(STDERR,"delete_path: relative path must not be NULL\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);

		if(SUCCESS != status)
		{
			echo(STDERR,"delete_path: failed to construct absolute path for \"%s\"\n",relative_path_to_tmpdir);
		}
	}

	if(SUCCESS == status)
	{
		if(lstat(m_text(absolute_path),&path_stat) != 0)
		{
			echo(STDERR,"delete_path: lstat failed for %s: %s\n",m_text(absolute_path),strerror(errno));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(S_ISDIR(path_stat.st_mode))
		{
			long sc_open_max = sysconf(_SC_OPEN_MAX);

			if(sc_open_max <= 0)
			{
				sc_open_max = 512;
			}

			if(nftw(m_text(absolute_path),nftw_remove_callback,(int)sc_open_max,FTW_DEPTH | FTW_PHYS) != 0)
			{
				echo(STDERR,"delete_path: nftw failed for %s: %s\n",m_text(absolute_path),strerror(errno));
				status = FAILURE;
			}
		} else if(remove(m_text(absolute_path)) != 0){
			echo(STDERR,"delete_path: remove failed for %s: %s\n",m_text(absolute_path),strerror(errno));
			status = FAILURE;
		}
	}

	m_del(absolute_path);

	return(status);
}
