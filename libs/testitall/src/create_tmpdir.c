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
Return create_tmpdir(memory *path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	const char *tmpdir = getenv("TMPDIR");
	char *directory_path_data_rewritable = NULL;
	size_t template_length = 0U;

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

	/* Start the template with the selected base directory */
	if(TRIUMPH & status)
	{
		status = m_copy_string(path,tmpdir);
	}

	if(TRIUMPH & status)
	{
		status = m_string_length(path,&template_length);
	}

	/* Insert a separator only when the base directory does not already end with '/' */
	if((TRIUMPH & status) && template_length > 0U)
	{
		const char *path_text = m_text(path);

		if('/' != path_text[template_length - 1U])
		{
			status = m_concat_literal(path,"/");
		}
	}

	/* Append the mkdtemp-compatible suffix with the configured application name */
	if(TRIUMPH & status)
	{
		status = m_concat_literal(path,TESTITALL_APP_NAME ".XXXXXX");
	}

	if(TRIUMPH & status)
	{
		status = m_string_length(path,&template_length);
	}

	/* Obtain a writable C string because mkdtemp rewrites the template in place */
	if(TRIUMPH & status)
	{
		directory_path_data_rewritable = m_data(char,path);

		if(NULL == directory_path_data_rewritable)
		{
			status = FAILURE;
		}
	}

	/* Create the directory and keep the resulting path in the same buffer */
	if((TRIUMPH & status) && NULL == mkdtemp(directory_path_data_rewritable))
	{
		status = FAILURE;
	}

	/* Synchronize cached string metadata after mkdtemp rewrites the template in place */
	if(TRIUMPH & status)
	{
		status = m_finalize_string(path,template_length,WRITE_TERMINATOR_ALWAYS);
	}

	if(CRITICAL & status)
	{
		call(m_del(path));
	}

	deliver(status);
}
