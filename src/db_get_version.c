/**
 * @file db_get_version.c
 * @brief Functions for checking metadata table existence and querying database version
 */

#include "precizer.h"

/**
 * @brief Retrieves database version from the metadata table
 *
 * @details Opens database connection, checks for metadata table existence
 *          and retrieves version number if available. Handles all necessary
 *          resource cleanup.
 *
 * @param[in] db_file_path Path to the SQLite database file
 * @param[out] db_version Pointer to store the retrieved version number
 *
 * @return Return status codes:
 *         - SUCCESS: Version retrieved successfully (may be 0 if not found)
 *         - FAILURE: Database error or invalid parameters
 */
Return db_get_version(
	int        *db_version,
	const char *db_file_path
){
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	bool table_exists = false;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameters: db_file_path\n");
		return(FAILURE);
	}

	/* Open database connection */
	if(SQLITE_OK != sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READONLY,NULL))
	{
		slog(ERROR,"Failed to open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	/* Check if metadata table exists */
	if(SUCCESS == status)
	{
		const char *check_query = "SELECT name FROM sqlite_master WHERE type='table' AND name='metadata';";

		if(SQLITE_OK != sqlite3_prepare_v2(db,check_query,-1,&stmt,NULL))
		{
			slog(ERROR,"Failed to prepare table existence check query: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(SQLITE_ROW == sqlite3_step(stmt))
		{
			table_exists = true;
		}
	}

	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	/* Get version if table exists */
	if(SUCCESS == status && table_exists == true)
	{
		const char *version_query = "SELECT db_version FROM metadata;";

		if(SQLITE_OK != sqlite3_prepare_v2(db,version_query,-1,&stmt,NULL))
		{
			slog(ERROR,"Failed to prepare version query: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			if(SQLITE_ROW == sqlite3_step(stmt))
			{
				*db_version = sqlite3_column_int(stmt,0);
				slog(TRACE,"Version number %d found in database\n",*db_version);

			} else {
				slog(TRACE,"No DB version data found in metadata table\n");
			}
		}

	} else if(SUCCESS == status){
		slog(TRACE,"Metadata table not found in database\n");
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
			status = FAILURE;
		}
	}

	return(status);
}
