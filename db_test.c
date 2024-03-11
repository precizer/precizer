#include "precizer.h"

/**
 *
 * Check up the database file
 *
 */
Return db_test
(
	Config *config
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

exit(0);

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	int rc = 0;

	slog(config,false,"Starting integrity check...");

	const char *sql = "PRAGMA integrity_check";

	rc = sqlite3_prepare_v2(config->db, sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(config,false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		sqlite3_int64 rows = -1;
		rows = sqlite3_column_int64(select_stmt,0);
		if(rows > 0)
		{
			config->db_already_exists = true;
		}
	}
	if(SQLITE_DONE != rc) {
		slog(config,false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	return(status);
}
