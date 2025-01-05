/**
 * @file db_consider_version_update.c
 * @brief
 */

#include "precizer.h"

/**
 * If the primary database has been modified and upgraded,
 * then store the current database version in the
 * metadata table
 *
 */
Return db_consider_version_update(void){
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. Store the primary database version is not required\n");
		return(status);
	} else if(config->dry_run == true && config->db_file_exists == false){
		slog(TRACE,"Dry Run mode is enabled. Store the primary database version is not required\n");
		return(status);
	}

	int db_version = 0;

	status = db_get_version(&db_version,config->db_file_path);

	if(SUCCESS == status)
	{
		if(config->something_has_been_changed == true)
		{
			if(db_version < CURRENT_DB_VERSION)
			{
				status = db_specify_version(config->db_file_path);

				if(SUCCESS == status)
				{
					/* Changes have been made to the database. Update
					  this in the global variable value. */
					config->something_has_been_changed = true;
				}
			}
		}
	}

	return(status);
}
