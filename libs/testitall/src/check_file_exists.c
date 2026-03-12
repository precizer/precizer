/**
 * @file check_file_exists.c
 * @brief File existence verification functionality
 */

#include "testitall.h"

/**
 * @brief Checks if a file exists
 *
 * @param[out] file_exists Pointer that will be set to true when the file exists, false otherwise
 * @param[in]  filename    Path to the file to check
 *
 * @return Return status indicating the result of operation:
 *         - SUCCESS after a successful check (even when the file is absent)
 *         - FAILURE when filename is NULL
 */
Return check_file_exists(
	bool       *file_exists,
	const char *filename)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	*file_exists = false;

	if(NULL == filename)
	{
		deliver(FAILURE);
	}

	if(0 == access(filename,F_OK))
	{
		*file_exists = true;
	}

	deliver(status);
}
