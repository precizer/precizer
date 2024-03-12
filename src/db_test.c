#include "precizer.h"

/**
 *
 * Check up the integrity of database file
 *
 */
Return db_test
(
	const char *db_file_path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// If the database has not in-memory type
	if(strcmp(db_file_path,":memory:") == 0)
	{
		return(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	sqlite3 *db = NULL;
	int rc = 0;

	// Default value
	bool database_is_ok = false;

	slog(false,"Starting of database file %s integrity check...\n",db_file_path);

	/* Open database */
	if(sqlite3_open(db_file_path, &db))
	{
		slog(false,"Can't open database: %s\n", sqlite3_errmsg(db));
		status = FAILURE;
	}

	const char *sql = "PRAGMA integrity_check";

	rc = sqlite3_prepare_v2(db, sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(db));
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
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	if(database_is_ok == true)
	{
		slog(false,"The database %s is in good condition\n",db_file_path);
	} else {
		slog(false,"ERROR: The database %s is in poor condition!\n",db_file_path);
		status = FAILURE;
	}

	sqlite3_close(db);

	return(status);
}
