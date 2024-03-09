#include "precizer.h"

Return db_delete_the_file_by_id
(
	Config *config,
	sqlite_int64 *ID,
	bool *first_iteration,
	const char *relative_path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	sqlite3_stmt *delete_stmt = NULL;
	int rc = 0;

	rc = sqlite3_prepare_v2(config->db, "DELETE FROM files WHERE ID=?1;", -1, &delete_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(config,false,"Can't prepare delete statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_int64(delete_stmt,1,*ID);
	if(SQLITE_OK != rc) {
		slog(config,false,"Error binding value in delete (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/* Execute SQL statement */
	if(sqlite3_step(delete_stmt) == SQLITE_DONE)
	{
		if (*first_iteration == true){
			*first_iteration = false;

			// Reflect changes in global
			config->something_has_been_changed = true;

			slog(config,false,"\033[1mThese files no longer exist and will be deleted against the DB %s:\n\033[m",config->db_file_name);
		}
		slog(config,false,"%s\n",relative_path);
	} else {
		slog(config,false,"Delete statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(delete_stmt);

	return(status);
}
