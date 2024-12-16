/**
 * @file file_check.h
 * @brief File existence verification functionality
 */

#include "testitall.h"

/**
 * @brief Checks if a file exists
 *
 * @param[in] filename    Path to the file to check
 * @param[in] file_exists True if the file exists of false if not
 *
 * @return Return status indicating the result of operation:
 *         - SUCCESS if finished successfully
 *         - FAILURE if any error
 */
Return check_file_exists(
	bool *file_exists,
	const char* filename
){
	Return status = SUCCESS;

	*file_exists = false;

	if(NULL == filename)
	{
		return(FAILURE);
	}

	if(0 == access(filename, F_OK))
	{
		*file_exists = true;
	}

	return(status);
}
