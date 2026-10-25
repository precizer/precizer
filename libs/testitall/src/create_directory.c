#include "testitall.h"
#include <errno.h>

/**
 * @brief Create one directory and accept an existing path that resolves to a directory
 *
 * Existing symlinks to directories are treated as valid path components
 *
 * @param[in] directory_path Absolute directory path
 *
 * @return 0 on success, non-zero on failure
 */
static int create_directory_if_missing(const char *directory_path)
{
	int status = 0;
	struct stat directory_stat = {0};

	if(directory_path == NULL)
	{
		status = -1;
	}

	if(status == 0 && mkdir(directory_path,0700) != 0 && errno != EEXIST)
	{
		status = -1;
	}

	if(status == 0 && stat(directory_path,&directory_stat) != 0)
	{
		status = -1;
	}

	if(status == 0 && !S_ISDIR(directory_stat.st_mode))
	{
		status = -1;
	}

	return(status);
}

/**
 * @brief Create directory tree by path relative to TMPDIR
 *
 * Empty path resolves to TMPDIR root. Existing directories are preserved.
 * Existing symlinks to directories in the TMPDIR prefix are accepted
 *
 * @param[in] relative_path_to_tmpdir Directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Directory tree exists after the call
 *         - FAILURE: Path construction or directory creation failed
 */
Return create_directory(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *absolute_path_data = NULL;
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		absolute_path_data = m_data(char,absolute_path);

		if(absolute_path_data == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		size_t absolute_path_length = strlen(absolute_path_data);

		while(absolute_path_length > 1U && absolute_path_data[absolute_path_length - 1U] == '/')
		{
			absolute_path_data[absolute_path_length - 1U] = '\0';
			absolute_path_length--;
		}
	}

	if(SUCCESS == status)
	{
		for(char *path_cursor = absolute_path_data + 1;
		        SUCCESS == status && *path_cursor != '\0';
		        path_cursor++)
		{
			if(*path_cursor == '/')
			{
				*path_cursor = '\0';

				if(create_directory_if_missing(absolute_path_data) != 0)
				{
					status = FAILURE;
				}
				*path_cursor = '/';
			}
		}
	}

	if(SUCCESS == status)
	{
		if(create_directory_if_missing(absolute_path_data) != 0)
		{
			status = FAILURE;
		}
	}

	m_del(absolute_path);

	return(status);
}
