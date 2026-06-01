/**
 * @file db_retrieve_version.c
 * @brief Functions for checking metadata table existence and retrieving database version
 */

#include "precizer.h"

/**
 * @brief Retrieve database version from the metadata table
 *
 * @details Opens database connection, checks for metadata table existence
 *          and retrieves the version number if available. Handles all necessary
 *          resource cleanup.
 *
 * @param[in] db_file_path Path to the SQLite database file
 * @param[out] db_version Pointer to store the retrieved version number
 *
 * @return Return status codes:
 *         - SUCCESS: Version retrieved successfully (may be 0 if not found)
 *         - FAILURE: Database error or invalid parameters
 */
Return db_retrieve_version(
	int        *db_version,
	const char *db_file_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *table_exists_stmt = NULL;
	sqlite3_stmt *version_stmt = NULL;
	bool table_exists = false;
	int rc = SQLITE_OK;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameters: db_file_path\n");
		provide(FAILURE);
	}

	/* Open database connection */
	rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READONLY,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	/* Check if metadata table exists */
	if(SUCCESS == status)
	{
		const char *check_query = "SELECT name FROM sqlite_master WHERE type='table' AND name='metadata';";

		rc = sqlite3_prepare_v2(db,check_query,-1,&table_exists_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to prepare table existence check query");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(SQLITE_ROW == sqlite3_step(table_exists_stmt))
		{
			table_exists = true;
		}
	}

	sqlite3_finalize(table_exists_stmt);

	/* Retrieve version if table exists */
	if(SUCCESS == status && table_exists == true)
	{
		const char *version_query = "SELECT db_version FROM metadata;";

		rc = sqlite3_prepare_v2(db,version_query,-1,&version_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to prepare version query");
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			if(SQLITE_ROW == sqlite3_step(version_stmt))
			{
				*db_version = sqlite3_column_int(version_stmt,0);
				slog(TRACE,"Version number %d found in database\n",*db_version);

			} else {
				slog(TRACE,"No DB version data found in metadata table\n");
			}
		}

	} else if(SUCCESS == status){
		slog(TRACE,"Metadata table not found in database\n");
	}

	/* Cleanup */
	sqlite3_finalize(version_stmt);

	if(db != NULL)
	{
		rc = sqlite3_close(db);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Warning: failed to close database");
			status = FAILURE;
		}
	}

	provide(status);
}
