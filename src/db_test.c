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

	// Don't do anything
	if(config->compare == true)
	{
		slog(TRACE,"Compare mode is enabled. Database verification for %s is skipped\n",db_file_path);
		return(status);

	} else if(config->dry_run == true && config->db_file_exists == false)
	{
		slog(TRACE,"Dry Run mode is enabled. Database verification for %s is skipped\n",db_file_path);
		return(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	sqlite3 *db               = NULL;
	int rc                    = 0;

	// Default value
	bool database_is_ok = false;

	// Extract file name from a path
	char *tmp = (char *)calloc(strlen(db_file_path) + 1,sizeof(char));

	if(tmp == NULL)
	{
		report("Memory allocation failed, requested size: %zu bytes",strlen(db_file_path) + 1 * sizeof(char));
		return(FAILURE);
	}

	strcpy(tmp,db_file_path);
	const char *db_file_name = basename(tmp);

	slog(EVERY,"Starting of database file %s integrity check...\n",db_file_name);

	int sqlite_open_flag = SQLITE_OPEN_READONLY;

	/* Open database */
	if(sqlite3_open_v2(db_file_path,&db,sqlite_open_flag,NULL))
	{
		slog(ERROR,"Can't open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	const char *sql = "PRAGMA integrity_check";

	rc = sqlite3_prepare_v2(db,sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Can't prepare select statement (%i): %s\n",rc,sqlite3_errmsg(db));
		status = FAILURE;
	}

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
	sqlite3_finalize(select_stmt);

	if(database_is_ok == true)
	{
		slog(EVERY,"Database %s has been verified and is in good condition\n",db_file_name);
	} else {
		slog(ERROR,"The database %s is in poor condition!\n",db_file_name);
		status = FAILURE;
	}

	sqlite3_close(db);

	free(tmp);

	return(status);
}
