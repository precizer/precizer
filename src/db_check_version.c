/**
 * @file db_check_version.c
 * @brief Database-version validation and upgrade entry point
 */

#include "precizer.h"

/**
 * @brief Check a database version and run required upgrades
 *
 * Retrieves the version stored in the database metadata, upgrades older
 * databases to `CURRENT_DB_VERSION`, and rejects databases created for a newer
 * application version. When a non-primary database is upgraded, the function
 * vacuums it after the upgrade. The primary database is left for the regular
 * session-end vacuum path
 *
 * @param[in] db_file_path Path to the SQLite database file being checked
 * @param[in] db_file_name Display name used in diagnostic messages
 * @return SUCCESS when the database is compatible or was upgraded, WARNING
 *         when the database requires a newer application, or FAILURE when
 *         version retrieval, upgrade, or post-upgrade vacuum fails
 */
Return db_check_version(
	const char *db_file_path,
	const char *db_file_name)
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

	/* Has the database been updated or not? */
	bool db_has_been_upgraded = false;

	/// Database Version Control. Each database file maintains
	/// a version number. This is essential for proper database upgrades
	/// and ensures full compatibility between newer application versions and
	/// legacy versions of DB.
	/// Zerro by default
	int db_version = 0;

	status = db_retrieve_version(&db_version,db_file_path);

	if(SUCCESS == status)
	{
		slog(TRACE,"The %s database file is version %d\n",db_file_name,db_version);
	} else {

		slog(ERROR,"Failed to retrieve database version\n");
	}

	if(SUCCESS == status)
	{
		if(db_version < CURRENT_DB_VERSION)
		{
			status = db_upgrade(&db_version,db_file_path,db_file_name);

			if(SUCCESS == status)
			{
				db_has_been_upgraded = true;
			} else {
				slog(ERROR,"Database %s upgrade failed\n",db_file_name);
			}

		} else if(db_version > CURRENT_DB_VERSION){
			slog(ERROR,"The database %s is designed to work with a newer version "
				"of the application and cannot be used with the old one. "
				"Please update %s application to the last version\n",db_file_name,APP_NAME);
			status = WARNING;
		} else {
			slog(TRACE,"The database %s is on version %d and does not require any upgrades\n",db_file_name,db_version);
		}
	}

	if(SUCCESS == status)
	{
		if(db_has_been_upgraded == true)
		{
			/* Check if the database being vacuumed is not the primary database.
			   The primary database doesn't need to be vacuumed during updates.
			   It will be vacuumed automatically before the Precizer
			   application session ends */
			if(strcmp(db_file_path,confstr(db_primary_file_path)) != 0)
			{
				/* Vacuum the database */
				status = db_vacuum(db_file_path);
			} else {
				slog(TRACE,"The primary database doesn't need to be vacuumed during updates\n");
			}
		}
	}

	provide(status);
}
