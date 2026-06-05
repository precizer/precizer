#include "precizer.h"

/**
 * @brief Validates the integrity of SQLite database file
 * @details Performs integrity check on the specified database file using either
 *          quick or full check based on configuration. Also verifies database
 *          version and upgrades if necessary.
 *
 * @param[in] db_file_path Path to the SQLite database file to validate
 * @return Return status code:
 *         - SUCCESS: Database validation passed successfully
 *         - FAILURE: Database validation failed or errors occurred
 */
Return db_integrity_check(const char *db_file_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3_stmt *select_stmt = NULL;
	sqlite3 *db = NULL;

	// Default value
	bool database_is_ok = false;

	const char *db_file_name = NULL;

	/* Extract file name from path */
	char *dirc = strdup(db_file_path);

	if(dirc == NULL)
	{
		serp("Memory allocation failed during strdup operation");
		provide(FAILURE);
	}

	db_file_name = basename(dirc);

	if(db_file_name == NULL)
	{
		report("basename() failed for path: %s",db_file_path);
		free(dirc);
		provide(FAILURE);
	}

	/* Check if verification should be skipped */
	if(config->dry_run == true && config->db_primary_file_exists == false)
	{
		slog(TRACE,"Dry Run mode is enabled. Database verification for %s is skipped\n",db_file_name);
		free(dirc);
		provide(SUCCESS);
	}

	/* Open database in read-only mode */
	slog(EVERY,"Starting database file %s integrity check…\n",db_file_name);
	int sqlite_open_flag = SQLITE_OPEN_READONLY;

	/* Open database */
	int rc = sqlite3_open_v2(db_file_path,&db,sqlite_open_flag,NULL);

	if(rc != SQLITE_OK)
	{
		log_sqlite_error(db,rc,NULL,"Can't open database");
		status = FAILURE;
	}

	/* Prepare integrity check statement */
	if(SUCCESS == status)
	{
		const char *sql = "PRAGMA integrity_check";

		if(config->db_check_level == QUICK)
		{
			sql = "PRAGMA quick_check";
			slog(TRACE,"The database verification level has been set to QUICK\n");
		} else {
			slog(TRACE,"The database verification level has been set to FULL\n");
		}

		rc = sqlite3_prepare_v2(db,sql,-1,&select_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Can't prepare select statement");
			status = FAILURE;
		}
	}

	/* Execute integrity check */
	if(SUCCESS == status)
	{
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			const char *response = (const char *)sqlite3_column_text(select_stmt,0);

			if(strcmp(response,"ok") == 0)
			{
				database_is_ok = true;
			}
		}

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	sqlite3_finalize(select_stmt);

	/* Report integrity check results */
	if(SUCCESS == status)
	{
		if(database_is_ok == true)
		{
			slog(EVERY,"Database %s has been verified and is in good condition\n",db_file_name);
		} else {
			slog(ERROR,"The database %s is in poor condition!\n",db_file_name);
			status = FAILURE;
		}
	}

	/* Cleanup resources */

	rc = sqlite3_close(db);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Warning: failed to close database");
		status = FAILURE;
	}

	/* Check database version if integrity check passed */
	if(SUCCESS == status)
	{
		/* Check the database version number and upgrade the database if necessary */
		status = db_check_version(db_file_path,db_file_name);
	}

	if(dirc != NULL)
	{
		free(dirc);
	}

	provide(status);
}
