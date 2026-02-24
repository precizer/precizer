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
	int rc = SQLITE_OK;

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
	rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Begin transaction */
		rc = sqlite3_exec(db,"BEGIN TRANSACTION",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to begin transaction");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Remove all from the table */
		rc = sqlite3_exec(db,"DELETE FROM metadata;",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to remove all from the table");
			status = FAILURE;
		}
	}

	/* Insert version number */
	if(SUCCESS == status)
	{
		rc = sqlite3_prepare_v2(db,"INSERT INTO metadata (db_version) VALUES(?);",-1,&stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to prepare insert query");
			status = FAILURE;
		} else {
			slog(TRACE,"The database version %d has been successfully stored in the DB\n",version);
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int(stmt,1,version);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to bind version number");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_step(stmt);

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to execute insert query");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Commit transaction */
		rc = sqlite3_exec(db,"COMMIT",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to commit transaction");
			status = FAILURE;
		} else {
			db_file_modified = true;

			if(strcmp(db_file_path,confstr(db_primary_file_path)) == 0)
			{
				/* Changes have been made to the database. Update
				   this in the global variable value. */
				config->db_primary_file_modified = true;
			}
		}
	}

	if(SUCCESS != status)
	{
		/* Attempt rollback */
		rc = sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);

		if(SQLITE_OK == rc)
		{
			slog(TRACE,"The transaction has been rolled back\n");
		} else {
			log_sqlite_error(db,rc,NULL,"Failed to rollback transaction");
		}
	}

	/* Cleanup */
	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	/* Cleanup */
	call(db_close(db,&db_file_modified));

	provide(status);
}
