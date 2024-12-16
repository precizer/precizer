#include "precizer.h"

/**
 *
 * Check up the integrity of database file
 *
 */
Return db_test(const char *db_file_path){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	sqlite3_stmt *select_stmt = NULL;
	sqlite3 *db               = NULL;
	int rc                    = 0;

	// Default value
	bool database_is_ok = false;

	const char *db_file_name = NULL;

	// Extract file name from a path
	char *dirc = strdup(db_file_path);

	if(dirc == NULL)
	{
		serp("Memory allocation failed during strdup operation");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		db_file_name = basename(dirc);

		if(db_file_name == NULL)
		{
			report("basename failed for path: %s", db_file_name);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		// Don't do anything and interrupt
		if(config->compare == true)
		{
			slog(TRACE,"Compare mode is enabled. Database verification for %s is skipped\n",db_file_name);
			free(dirc);
			return(SUCCESS);

		} else if(config->dry_run == true && config->db_file_exists == false)
		{
			slog(TRACE,"Dry Run mode is enabled. Database verification for %s is skipped\n",db_file_name);
			free(dirc);
			return(SUCCESS);
		}
	}

	if(SUCCESS != status)
	{
		free(dirc);
		return(status);
	}

	if(SUCCESS == status)
	{

		slog(EVERY,"Starting of database file %s integrity check...\n",db_file_name);

		int sqlite_open_flag = SQLITE_OPEN_READONLY;

		/* Open database */
		if(sqlite3_open_v2(db_file_path,&db,sqlite_open_flag,NULL))
		{
			slog(ERROR,"Can't open database: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const char *sql = "PRAGMA quick_check";
		if(config->db_check_level == FULL)
		{
			sql = "PRAGMA integrity_check";
			slog(TRACE,"The database verification level has been set to FULL\n");
		} else {
			slog(TRACE,"The database verification level has been set to QUICK\n");
		}

		rc = sqlite3_prepare_v2(db,sql,-1,&select_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Can't prepare select statement (%i): %s\n",rc,sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

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
			slog(ERROR,"Select statement didn't finish with DONE (%i): %s\n",rc,sqlite3_errmsg(db));
			status = FAILURE;
		}
	}
	sqlite3_finalize(select_stmt);

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

	sqlite3_close(db);

	free(dirc);

	return(status);
}
