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
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *tmp_dir = NULL;
	size_t path_len = 0;

	if(SUCCESS == status)
	{
		if((filename == NULL) || (full_path == NULL))
		{
			echo(STDERR,"construct_path: filename and output buffer must not be NULL\n");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		tmp_dir = getenv("TMPDIR");

		if(NULL == tmp_dir)
		{
			echo(STDERR,"construct_path: TMPDIR is not set for \"%s\"\n",filename);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		path_len = strlen(tmp_dir) + strlen(filename) + 2;   // +2 for '/' and '\0'
		status = resize(full_path,path_len);

		if(SUCCESS != status)
		{
			echo(STDERR,"construct_path: failed to allocate %zu bytes for \"%s\"\n",path_len,filename);
		}
	}

	if(SUCCESS == status)
	{
		char *path_data = data(char,full_path);

		if(path_data == NULL)
		{
			echo(STDERR,"construct_path: resized buffer has no data pointer for \"%s\"\n",filename);
			status = FAILURE;
		} else {
			int print_result = snprintf(path_data,path_len,"%s/%s",tmp_dir,filename);

			if(print_result < 0 || (size_t)print_result >= path_len)
			{
				echo(STDERR,"construct_path: snprintf failed for \"%s\"\n",filename);
				status = FAILURE;
				path_data[0] = '\0';
			}
		}
	}

	deliver(status);
}
