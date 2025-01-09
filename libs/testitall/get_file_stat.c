#include "testitall.h"
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief Check if file exists and get its stats
 *
 * @param path[in]      Path to file to check
 * @param stat_buf[out] Pointer to stat structure to be filled with file info
 *
 * @return Return status:
 *         - SUCCESS:  File exists and stat structure filled
 *         - FAILURE:  File not found or stat failed
 */
Return get_file_stat(
	const char  *path,
	struct stat *stat_buf
){
	Return status = SUCCESS;

	if((NULL == path) || (NULL == stat_buf))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = stat(path,stat_buf);

		if(-1 == rc)
		{
			report("File not found: %s",path);
			status = FAILURE;
		} else if(rc < 0){
			report("Stat of %s failed with error code: %d",path,rc);
			status = FAILURE;
		}
	}

	return(status);
}
