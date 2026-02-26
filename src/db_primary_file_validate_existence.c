#include "precizer.h"

/**
 * @brief Validates the existence of the primary database file
 * @details Checks if the primary database file exists and is accessible. Updates the
 *    global config->db_primary_file_exists flag based on the check result.
 *    The function attempts to access the file using the path stored
 *    in the global configuration config->db_primary_file_path
 *
 * @return Return status code indicating the operation result:
 *    - SUCCESS: Check completed successfully
 *    - FAILURE: File check operation failed
 *
 * @note This function only verifies file existence and basic accessibility.
 *    It does not validate file format or content integrity.
 *
 * @see config->db_primary_file_exists
 * @see config->db_primary_file_path
 */
Return db_primary_file_validate_existence(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	// Primary DB file exists or not
	config->db_primary_file_exists = false;
	const char *db_primary_file_path = confstr(db_primary_file_path);

	// Path is initialized in db_determine_name() and must contain at least one
	// real character beyond the trailing '\0' terminator
	if(conf(db_primary_file_path)->length <= 1)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		char *db_file_full_path = strdup(db_primary_file_path);

		if(db_file_full_path == NULL)
		{
			report("Memory allocation failed for database path copy");
			status = FAILURE;
		}

		if(SUCCESS != status)
		{
			provide(status);
		}

		char *db_file_dir = dirname(db_file_full_path);

		if(NOT_FOUND == file_availability(db_file_dir,SHOULD_BE_A_DIRECTORY))
		{
			slog(ERROR,"Unable to create database file. Directory %s not found\n",db_file_dir);
			status = FAILURE;
		}

		free(db_file_full_path);

		if(SUCCESS == status)
		{
			if(EXISTS == file_availability(db_primary_file_path,SHOULD_BE_A_FILE))
			{
				config->db_primary_file_exists = true;

				int rc = stat(db_primary_file_path,&config->db_file_stat);

				if(rc < 0)
				{
					report("Stat of %s failed with error code: %d",db_primary_file_path,rc);
					status = FAILURE;
				}

			} else {
				config->db_primary_file_exists = false;
			}
		}
	}

	provide(status);
}
