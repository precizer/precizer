#include "precizer.h"

/**
 *
 * This function remove information about a specific
 * file from the database by its unique db ID
 *
 */
Return db_delete_the_file_by_id(
	sqlite_int64 *ID,
	bool         *first_iteration,
	const bool   *clean_ignored,
	const char   *relative_path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	sqlite3_stmt *delete_stmt = NULL;
	int rc = 0;

	const char *sql = "DELETE FROM files WHERE ID=?1;";

	// Don't do anything in case of --dry-run
	if(config->dry_run == true)
	{
		sql = "SELECT ID FROM files WHERE ID=?1;";
	}

	rc = sqlite3_prepare_v2(config->db,sql,-1,&delete_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Can't prepare delete statement (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_int64(delete_stmt,1,*ID);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Error binding value in delete (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	int sql_return = SQLITE_DONE;

	// Select instead Delete in Dry Run mode
	if(config->dry_run == true)
	{
		sql_return = SQLITE_ROW;
	}

	/* Execute SQL statement */
	if(sqlite3_step(delete_stmt) == sql_return)
	{
		if(*first_iteration == true)
		{

			if(config->update == true && config->something_has_been_changed == false)
			{
				slog(EVERY,"The " BOLD "--update" RESET " option has been used, so the information about files will be deleted against the database %s\n",config->db_file_name);
			}

			*first_iteration = false;

			// Reflect changes in global
			config->something_has_been_changed = true;

			slog(EVERY,BOLD "These files are no longer exist or ignored and will be deleted against the DB %s:" RESET "\n",config->db_file_name);
		}

		if(*clean_ignored == true)
		{
			slog(EVERY,"clean ignored %s\n",relative_path);
		} else {
			slog(EVERY,"%s\n",relative_path);
		}
	} else {
		slog(ERROR,"Delete statement didn't return right code %d (%i): %s\n",sql_return,rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(delete_stmt);

	return(status);
}
