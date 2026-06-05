#include "testitall.h"
#include <limits.h>
#include <string.h>

/**
 * @brief Write the parent directory of the current working directory into a memory descriptor
 *
 * The current working directory is copied into the provided memory descriptor,
 * trailing slashes are removed, and then the parent directory is kept there
 *
 * Example: if CWD is "/tmp/precizer/run///", the function writes "/tmp/precizer"
 * The root path "/" is preserved as "/"
 *
 * @param path Destination char MEMORY_STRING descriptor
 * @return SUCCESS on success, FAILURE on error
 */
Return determine_origin_dir(memory *path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Heap-allocated current working directory returned by libc */
	char *cwd = NULL;

	/* Read-only C string view of the destination descriptor payload */
	const char *path_text_readonly = NULL;

	/* Address of the last path separator inside path_text_readonly */
	const char *last_slash = NULL;

	/* Visible string length measured before trimming trailing slashes */
	size_t path_length = 0U;

	/* Visible string length after trimming trailing slashes */
	size_t trimmed_path_length = 0U;

	/* Visible parent-directory length without the trailing terminator */
	size_t parent_length = 0U;

	/* Whether the parent directory is the filesystem root */
	bool parent_is_root = false;

#if defined(__GLIBC__)
	cwd = get_current_dir_name();
#else
	// Portable fallback for evilOS/BSD.
	cwd = getcwd(NULL,0);
#endif

	if(NULL == cwd)
	{
		status = FAILURE;
	}

	if(TRIUMPH & status)
	{
		status = m_copy_string(path,cwd);
	}

	if(TRIUMPH & status)
	{
		status = m_string_length(path,&path_length);
	}

	if(TRIUMPH & status)
	{
		path_text_readonly = m_text(path);
		trimmed_path_length = path_length;
	}

	/*
	 * Use the visible string length instead of descriptor capacity
	 * Stop at length 1 so "/" does not become an empty string
	 */
	if(TRIUMPH & status)
	{
		while(trimmed_path_length > 1U &&
		        path_text_readonly[trimmed_path_length - 1U] == '/')
		{
			trimmed_path_length--;
		}

		status = m_string_truncate(path,trimmed_path_length);
	}

	if(TRIUMPH & status)
	{
		path_text_readonly = m_text(path);
		last_slash = strrchr(path_text_readonly,'/');

		if(NULL == last_slash)
		{
			status = FAILURE;
		}

		if(TRIUMPH & status)
		{
			parent_is_root = (last_slash == path_text_readonly);
		}
	}

	if((TRIUMPH & status) && parent_is_root == true)
	{
		status = m_copy_literal(path,"/");
	}

	if((TRIUMPH & status) && parent_is_root == false)
	{
		parent_length = (size_t)(last_slash - path_text_readonly);

		status = m_string_truncate(path,parent_length);
	}

	if((TRIUMPH & status) && parent_is_root == false)
	{
		/* Visible parent-directory length with a terminator for shrink-to-fit */
		const size_t parent_size = parent_length + 1U;

		status = m_resize(path,parent_size,RELEASE_UNUSED);
	}

	if(NULL != cwd)
	{
		free(cwd);
		cwd = NULL;
	}

	if(CRITICAL & status)
	{
		call(m_del(path));
	}

	deliver(status);
}
