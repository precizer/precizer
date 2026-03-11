#include "testitall.h"

/**
 * @brief Create a unique temporary directory and return its path
 *
 * Uses `$TMPDIR` when it is set and non-empty, otherwise falls back to
 * `P_tmpdir` when available and non-empty, then to `/tmp`
 * The path template has the format `<base>/<application-name>.XXXXXX`
 * The application name comes from `APP_NAME` when it is defined, otherwise
 * from `TESTITALL_APP_NAME`
 * The template is assembled in the caller-provided memory descriptor and then
 * passed to mkdtemp()
 *
 * @param path Destination memory descriptor initialized for char elements
 * @return SUCCESS on success, FAILURE on error
 */
Return create_tmpdir(
	memory *path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	const char *tmpdir = getenv("TMPDIR");
	const char template_suffix[] = TESTITALL_APP_NAME ".XXXXXX";
	size_t tmpdir_length = 0U;
	char *directory_path = NULL;

	/* Prefer the process-defined temporary directory when it is available */
	if(NULL == tmpdir || '\0' == tmpdir[0])
	{
		#ifdef P_tmpdir
		tmpdir = P_tmpdir;
		#else
		tmpdir = NULL;
		#endif
	}

	/* Fall back to the conventional POSIX temporary directory as a last resort */
	if(NULL == tmpdir || '\0' == tmpdir[0])
	{
		tmpdir = "/tmp";
	}

	tmpdir_length = strlen(tmpdir);

	/* Start the template with the selected base directory */
	if(SUCCESS != copy_literal(path,tmpdir))
	{
		del(path);
		deliver(FAILURE);
	}

	/* Insert a separator only when the base directory does not already end with '/' */
	if('/' != tmpdir[tmpdir_length - 1U])
	{
		if(SUCCESS != concat_literal(path,"/"))
		{
			del(path);
			deliver(FAILURE);
		}
	}

	/* Append the mkdtemp-compatible suffix with the configured application name */
	if(SUCCESS != concat_literal(path,template_suffix))
	{
		del(path);
		deliver(FAILURE);
	}

	/* Obtain a writable C string because mkdtemp rewrites the template in place */
	directory_path = data(char,path);

	if(NULL == directory_path)
	{
		del(path);
		deliver(FAILURE);
	}

	/* Create the directory and keep the resulting path in the same buffer */
	if(NULL == mkdtemp(directory_path))
	{
		del(path);
		deliver(FAILURE);
	}

	deliver(SUCCESS);
}
