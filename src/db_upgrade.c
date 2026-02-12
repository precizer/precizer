/**
 * @file db_upgrade.c
 * @brief Orchestrates upgrades of legacy database versions to the current one.
 */

#include "precizer.h"

/**
 * @brief Upgrade database to CURRENT_DB_VERSION.
 *
 * The upgrade path is sequential: first migrate to version 1 (for legacy v0),
 * then migrate to version 4 from the currently reached version.
 *
 * @param[in,out] db_version Current database version from metadata; updated
 * to each successfully stored target version.
 * @param[in] db_file_path Path to the SQLite database file.
 * @param[in] db_file_name Database file name used for logging.
 * @return Return status code.
 */
Return db_upgrade(
	int        *db_version,
	const char *db_file_path,
	const char *db_file_name)
{
	Return status = SUCCESS;

	slog(EVERY,"The database %s file has an outdated version %d and requires updating to the version %d\n",db_file_name,*db_version,CURRENT_DB_VERSION);
	slog(EVERY,"Warning! The update will be performed in transaction mode for database safety\n");
	slog(EVERY,"Caution! After the update, the database file will not work correctly with old versions of %s. "
		"Please update all copies of the application that will use the new database version!\n",APP_NAME);

	if(config->update == false)
	{
		slog(ERROR,"Program execution cannot continue. Database update required. Use the " BOLD "--update" RESET " flag to perform this action\n");
		provide(WARNING);
	}

	/* Sequentially upgrade through versions */

	/* This legacy can be removed in 2034 (10-year Long-Term Support) */
	if(*db_version < 1)
	{
		const int from_version = *db_version;

		slog(TRACE,"Migration from version %d to version 1 started\n",*db_version);
		status = db_migrate_from_0_to_1(db_file_path);

		if(SUCCESS == status)
		{
			slog(TRACE,"Store the database version 1 in the metadata table\n");
			status = db_specify_version(db_file_path,1);

			if(SUCCESS == status)
			{
				*db_version = 1;
			}
		}

		if(SUCCESS == status)
		{
			slog(TRACE,"Migration from version %d to version 1 completed\n",from_version);
		}
	}

	/* This legacy can be removed in 2036 (10-year Long-Term Support) */
	if(SUCCESS == status && *db_version < 4)
	{
		const int from_version = *db_version;

		slog(TRACE,"Migration from version %d to version 4 started\n",*db_version);
		status = db_migrate_to_version_4(db_file_path);

		if(SUCCESS == status)
		{
			slog(TRACE,"Store the database version 4 in the metadata table\n");
			status = db_specify_version(db_file_path,4);

			if(SUCCESS == status)
			{
				*db_version = 4;
			}
		}

		if(SUCCESS == status)
		{
			slog(TRACE,"Migration from version %d to version 4 completed\n",from_version);
		}
	}

	if(SUCCESS == status)
	{
		slog(EVERY,"The database has been successfully upgraded\n");
	}

	provide(status);
}
