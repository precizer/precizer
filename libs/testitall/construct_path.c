#include "testitall.h"

/**
 * @brief Constructs full file path by combining TMPDIR environment variable with filename
 *
 * @param[in] filename Name of the file to append to TMPDIR path
 * @param[out] full_path Pointer to string pointer that will store the allocated path
 * @return Return Status of the operation
 */
Return construct_path(
	const char *filename,
	char       **full_path
){
	Return status = SUCCESS;
	const char *tmp_dir = NULL;
	size_t path_len = 0;

	if(SUCCESS == status)
	{
		if(NULL == filename || NULL == full_path)
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
		*full_path = (char *)malloc(path_len);

		if(NULL == *full_path)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int print_result = snprintf(*full_path,path_len,"%s/%s",tmp_dir,filename);

		if(print_result < 0 || (size_t)print_result >= path_len)
		{
			free(*full_path);
			*full_path = NULL;
			status = FAILURE;
		}
	}

	return(status);
}
