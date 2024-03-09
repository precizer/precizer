#include "precizer.h"

Return db_vacuum
(
	const Config *config
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true){
		return(status);
	}

	slog(config,false,"Start vacuuming...\n");

	int rc;

	/* Create SQL statement */
	const char *sql = "pragma optimize;" \
	                  "VACUUM;";

	/* Execute SQL statement */
	rc = sqlite3_exec(config->db, sql, NULL, NULL, NULL);
	if(rc!= SQLITE_OK ){
		slog(config,false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	} else {
		slog(config,false,"The database has been vacuumed\n");
	}

	return(status);
}
