/**
 * @file db_get_version.c
 * @brief Functions for checking metadata table existence and querying database version
 */

#include "precizer.h"

/**
 * @brief Checks if metadata table exists and retrieves database version
 *
 * @details This function performs two sequential operations:
 *          1. Checks for the existence of the "metadata" table in the database
 *          2. If the table exists, retrieves the database version number
 *          The database connection is accessed through the global config structure
 *
 * @post On SUCCESS: config->db_version will contain either the retrieved version number,
 *       or will be set to 0 if no version is found
 *
 * @return Return status codes:
 *         - SUCCESS: Table exists and version retrieved successfully
 *         - FAILURE: SQL error occurred or invalid data retrieved
 */
Return db_get_version(void){

	Return status = SUCCESS;
	sqlite3_stmt *stmt = NULL;
	bool table_exists = false;
	const char *check_query = "SELECT name FROM sqlite_master WHERE type='table' AND name='metadata';";

	/* Check if table exists */
	if(SQLITE_OK != sqlite3_prepare_v2(config->db,check_query,-1,&stmt,NULL))
	{
		status = FAILURE;
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

		if(SQLITE_OK != sqlite3_prepare_v2(config->db,version_query,-1,&stmt,NULL))
		{
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			if(SQLITE_ROW == sqlite3_step(stmt))
			{
				config->db_version = sqlite3_column_int(stmt,0);
			} else {
				config->db_version = 0;
			}
		}
	}

	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	return(status);
}

