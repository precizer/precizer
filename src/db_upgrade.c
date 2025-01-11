/**
 * @file db_upgrade.c
 * @brief
 */

#include "precizer.h"

/**
 * @brief Upgrades database to the current version
 * @param[in] db_file_path Path to the SQLite database file
 * @param[out] db_version Pointer to store the retrieved version number
 * @return Return status code
 */
Return db_upgrade(
	const int  *db_version,
	const char *db_file_path,
	const char *db_file_name
){
	Return status = SUCCESS;

	slog(EVERY,"The database %s file has an outdated version %d and requires updating to the version %d\n",db_file_name,*db_version,CURRENT_DB_VERSION);
	slog(EVERY,"Caution! The update will be performed in transaction mode for database safety\n");
	slog(EVERY,"Caution! After the update, the database file will not work correctly with old versions of %s. "
		"Please update all copies of the application that will use the new database version!\n",APP_NAME);

	if(config->update == false)
	{
		slog(ERROR,"Program execution cannot continue. Database update required. Use the --update flag to perform this action\n");
		return(FAILURE);
	}

	/* Sequentially upgrade through versions */
	if(*db_version < 1)
	{
		slog(TRACE,"Migration from version 0 to version 1 started\n");
		status = migrate_from_0_to_1(db_file_path);

		if(SUCCESS == status)
		{
			slog(TRACE,"Store the current database version in the metadata table\n");
			status = db_specify_version(db_file_path);
		}

		if(SUCCESS == status)
		{
			slog(TRACE,"Migration from version 0 to version 1 completed\n");
		}
	}

#if 0
	if(SUCCESS == status && *db_version < 2)
	{
		slog(TRACE,"Migration from version 1 to version 2 started\n");
		status = migrate_from_1_to_2(&db_version,db_file_path,db_file_name);

		if(SUCCESS == status)
		{
			slog(TRACE,"Migration from version 1 to version 2 completed\n");
		}
	}
#endif

	if(SUCCESS == status)
	{
		slog(EVERY,"The database has been successfully upgraded\n");
	}

	return(status);
}

