#include "precizer.h"

/**
 *
 * Check whether the database already exists or not yet
 *
 */
Return db_already_exists(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	int rc = 0;

	// Default value
	config->db_already_exists = false;

	const char *sql_db_already_exists = "SELECT COUNT(*) FROM paths";

	rc = sqlite3_prepare_v2(config->db, sql_db_already_exists, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
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
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	// Stop if the update flag is not set but the database has already been created previously
	if(config->db_already_exists == true)
	{
		if(config->update == true)
		{
			slog(true,"The database has already been created in the past\n");
		} else {
			slog(false,"The database %s has been created in the past and already contains" \
			                  " data with files and their checksums. Use the \033[1m--update\033[m option if" \
			                  " there is full confidence that update the content of the database"  \
			                  " is really need and the information about those files which was changed," \
			                  " removed or added should be deleted or updated against DB.\n",config->db_file_name);
			status = WARNING;
			return(status);
		}
	}

	return(status);
}
