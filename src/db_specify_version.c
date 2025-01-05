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
Return db_specify_version(const char *db_file_path){
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameter: db_file_path\n");
		return(FAILURE);
	}

	if(global_interrupt_flag == true)
	{
		slog(TRACE,"The program has been gracefully terminated. Store the current database version is not required\n");
		return(status);
	}

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Store the current database version is not required\n");
		return(status);
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
			slog(TRACE,"The database version %d has been successfully stored in the DB\n",CURRENT_DB_VERSION);
		}
	}

	if(SUCCESS == status)
	{
		if(SQLITE_OK != sqlite3_bind_int(stmt,1,CURRENT_DB_VERSION))
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
		}
	}

	/* Cleanup */
	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	if(db != NULL)
	{
		if(SQLITE_OK != sqlite3_close(db))
		{
			slog(ERROR,"Warning: failed to close database: %s\n",sqlite3_errmsg(db));
			/* Don't change status as the operation was otherwise successful */
		}
	}

	return(status);
}

