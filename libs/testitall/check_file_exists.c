/**
 * @file file_check.h
 * @brief File existence verification functionality
 */

#ifndef FILE_CHECK_H
#define FILE_CHECK_H

#include "testitall.h"

/**
 * @brief Checks if a file exists
 *
 * @param[in] filename    Path to the file to check
 *
 * @return Return status indicating the result of operation:
 *         - SUCCESS if file exists
 *         - FAILURE if file doesn't exist or path is invalid
 */
Return check_file_exists(
	const char* filename
){
	Return status = SUCCESS;

	if(NULL == filename)
	{
		return(FAILURE);
	}

	if(0 != access(filename, F_OK))
	{
		status = FAILURE;
	}

	return(status);
}

#endif /* FILE_CHECK_H */
