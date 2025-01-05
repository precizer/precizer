/**
 * @file db_check_version.c
 * @brief
 */

#include "precizer.h"

/**
 * @brief Checks database version and initiates upgrade if needed
 *
 */
Return db_check_version(
	const char *db_file_path,
	const char *db_file_name
){
	Return status = SUCCESS;

	/// Database Version Control. Each database file maintains
	/// a version number. This is essential for proper database upgrades
	/// and ensures full compatibility between newer application versions and
	/// legacy versions of DB.
	/// Zerro by default
	int db_version = 0;

	status = db_get_version(&db_version,db_file_path);

	if(SUCCESS == status)
	{
		slog(TRACE,"The %s database file is version %d\n",db_file_name,db_version);
	} else {

		slog(ERROR,"Failed to get database version\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(db_version < CURRENT_DB_VERSION)
		{
			status = db_upgrade(&db_version,db_file_path,db_file_name);

			if(SUCCESS != status)
			{
				slog(ERROR,"Database %s upgrade failed\n",db_file_name);
				status = FAILURE;
			}

		} else if(db_version > CURRENT_DB_VERSION){
			slog(ERROR,"The database %s is designed to work with a newer version "
				"of the application and cannot be used with the old one. "
				"Please update %s to the last version\n",db_file_name,APP_NAME);
			status = FAILURE;
		} else {
			slog(TRACE,"The database %s is on version %d and does not require any upgrades\n",db_file_name,db_version);
		}
	}

	return(status);
}

