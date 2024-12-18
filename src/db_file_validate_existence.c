#include "precizer.h"

/**
 * @brief Validates the existence of the database file
 * @details Checks if the database file exists and is accessible. Updates the
 *    global config->db_file_exists flag based on the check result.
 *    The function attempts to access the file using the path stored
 *    in the global configuration config->db_file_path
 *
 * @return Return status code indicating the operation result:
 *    - SUCCESS: Check completed successfully
 *    - FAILURE: File check operation failed
 *
 * @note This function only verifies file existence and basic accessibility.
 *    It does not validate file format or content integrity.
 *
 * @see config->db_file_exists
 * @see config->db_file_path
 */
Return db_file_validate_existence(void){
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	// DB file exists or not
	config->db_file_exists = false;

	// The variable is defined in db_determine_name()
	// and must not be empty
	if(config->db_file_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(EXISTS == file_availability(config->db_file_path,SHOULD_BE_A_FILE))
		{
			config->db_file_exists = true;

			int rc = stat(config->db_file_path,&(config->db_file_stat));

			if(rc < 0)
			{
				report("Stat of %s failed with error code: %d",config->db_file_path,rc);
				status = FAILURE;
			}

		} else {
			config->db_file_exists = false;
		}
	}

	return(status);
}
