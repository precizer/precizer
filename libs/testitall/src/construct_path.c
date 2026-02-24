#include "testitall.h"

/**
 * @brief Constructs full file path by combining TMPDIR environment variable with filename
 *
 * @param[in]  filename  Name of the file to append to TMPDIR path
 * @param[out] full_path Managed memory buffer that will be resized to hold the constructed path
 * @return Return Status of the operation
 */
Return construct_path(
	const char *filename,
	memory     *full_path)
{
	Return status = SUCCESS;
	const char *tmp_dir = NULL;
	size_t path_len = 0;

	if(SUCCESS == status)
	{
		if((filename == NULL) || (full_path == NULL))
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		tmp_dir = getenv("TMPDIR");

		if(NULL == tmp_dir)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		path_len = strlen(tmp_dir) + strlen(filename) + 2;   // +2 for '/' and '\0'
		status = resize(full_path,path_len);
	}

	if(SUCCESS == status)
	{
		char *path_data = getstring(full_path);

		if(path_data == NULL)
		{
			status = FAILURE;
		} else {
			int print_result = snprintf(path_data,path_len,"%s/%s",tmp_dir,filename);

			if(print_result < 0 || (size_t)print_result >= path_len)
			{
				status = FAILURE;
				path_data[0] = '\0';
			}
		}
	}

	deliver(status);
}
