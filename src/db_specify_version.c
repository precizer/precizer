/**
 * @file db_specify_version.c
 * @brief
 */

#include "precizer.h"

/**
 * @brief Store the current database version in the metadata table
 *
 * @details Opens database connection and sets version number to CURRENT_DB_VERSION
 *          constant in the metadata table. Handles all necessary resource cleanup.
 *
 * @param[in] db_file_path Path to the SQLite database file
 *
 * @return Return status codes:
 *         - SUCCESS: Version set successfully
 *         - FAILURE: Database error or invalid parameters
 */
Return db_specify_version(
	const char *db_file_path,
	int        version)
{
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	bool db_file_modified = false;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameter: db_file_path\n");
		provide(FAILURE);
	}

	if(global_interrupt_flag == true)
	{
		slog(TRACE,"The program has been gracefully terminated. Store the current database version is not required\n");
		provide(status);
	}

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Store the current database version is not required\n");
		provide(status);
	}

	/* Open database connection */
	if(SQLITE_OK != sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE,NULL))
	{
		slog(ERROR,"Failed to open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	/* Insert version number */
	if(SUCCESS == status)
	{
		const char *insert_query = "REPLACE INTO metadata (db_version) VALUES (?);";

		if(SQLITE_OK != sqlite3_prepare_v2(db,insert_query,-1,&stmt,NULL))
		{
			slog(ERROR,"Failed to prepare insert query: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		} else {
			slog(TRACE,"The database version %d has been successfully stored in the DB\n",version);
		}
	}

	if(SUCCESS == status)
	{
		if(SQLITE_OK != sqlite3_bind_int(stmt,1,version))
		{
			slog(ERROR,"Failed to bind version number: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(SQLITE_DONE != sqlite3_step(stmt))
		{
			slog(ERROR,"Failed to execute insert query: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		} else {
			db_file_modified = true;

			if(strcmp(db_file_path,config->db_primary_file_path) == 0)
			{
				/* Changes have been made to the database. Update
				   this in the global variable value. */
				config->db_primary_file_modified = true;
			}
		}
	}

	/* Cleanup */
	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	/* Cleanup */
	if(SUCCESS == status)
	{
		status = db_close(db,&db_file_modified);
	}

	provide(status);
}
