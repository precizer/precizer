#include "precizer.h"

/**
 *
 * The VACUUM command rebuilds the database file,
 * repacking it into a minimal amount of disk space.
 *
 */
Return db_vacuum(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		slog(TRACE,"Compare mode is enabled. Vacuuming is not required for the main database\n");
		return(status);

	} else if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Vacuuming is not required for the main database\n");
		return(status);
	}

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		return(status);
	}

	slog(EVERY,"Start vacuuming...\n");

	int rc;

	/* Create SQL statement */
	const char *sql = "pragma optimize;" \
	        "VACUUM;";

	/* Execute SQL statement */
	rc = sqlite3_exec(config->db,sql,NULL,NULL,NULL);

	if(rc!= SQLITE_OK)
	{
		slog(ERROR,"Can't execute (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	} else {
		slog(EVERY,"The main database has been vacuumed\n");
	}

	return(status);
}
