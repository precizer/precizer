#include "precizer.h"

/**
 *
 * This function remove information about a specific
 * file from the database by its unique db ID
 *
 */
Return db_delete_the_file_by_id
(
	sqlite_int64 *ID,
	bool *first_iteration,
	const bool *clean_ignored,
	const char *relative_path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything in case of --dry_run
	if(config->dry_run == true)
	{
		return(status);
	}

	sqlite3_stmt *delete_stmt = NULL;
	int rc = 0;

	rc = sqlite3_prepare_v2(config->db, "DELETE FROM files WHERE ID=?1;", -1, &delete_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare delete statement (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_int64(delete_stmt,1,*ID);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in delete (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/* Execute SQL statement */
	if(sqlite3_step(delete_stmt) == SQLITE_DONE)
	{
		if (*first_iteration == true){

			if(config->update == true && config->something_has_been_changed == false)
			{
				slog(false,"The \033[1m--update\033[m option has been used, so the information about files will be deleted against the database %s\n",config->db_file_name);
			}

			*first_iteration = false;

			// Reflect changes in global
			config->something_has_been_changed = true;

			slog(false,"\033[1mThese files are ignored or no longer exist and will be deleted against the DB %s:\n\033[m",config->db_file_name);
		}
		if(*clean_ignored == true)
		{
			slog(false,"clean ignored %s\n",relative_path);
		} else {
			slog(false,"%s\n",relative_path);
		}
	} else {
		slog(false,"Delete statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(delete_stmt);

	return(status);
}
