#include "precizer.h"

/**
 *
 * The VACUUM command rebuilds the database file,
 * repacking it into a minimal amount of disk space.
 *
 */
Return db_vacuum(const char *db_file_path)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	sqlite3 *db = NULL;
	char *err_msg = NULL;
	bool db_is_primary = false;
	bool db_file_modified = false;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameters: db_file_path\n");
		provide(FAILURE);
	}

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. The database doesn't require vacuuming\n");
		provide(status);
	}

	if(strcmp(db_file_path,config->db_primary_file_path) == 0)
	{
		db_is_primary = true;
	}

	/* Open database in safe mode */
	int rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Failed to open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Create SQL statement */
		const char *sql =
		        "PRAGMA analyze;"
		        "PRAGMA optimize;"
		        "VACUUM;"
		        "PRAGMA analyze;"
		        "PRAGMA optimize;";

		if(db_is_primary == true)
		{
			slog(EVERY,"Start vacuuming the primary database…\n");
		} else {
			slog(EVERY,"Start vacuuming…\n");
		}

		/* Execute SQL statement */
		rc = sqlite3_exec(db,sql,NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Can't execute (%i): %s, %s\n",rc,sqlite3_errmsg(db),err_msg);
			sqlite3_free(err_msg);
			status = FAILURE;
		} else {
			db_file_modified = true;

			if(db_is_primary == true)
			{
				slog(EVERY,"The primary database has been vacuumed\n");
			} else {
				slog(EVERY,"The database has been vacuumed\n");
			}
		}
	}

	if(SUCCESS == status)
	{
		/**
		 *
		 * If the database being updated is the primary one, adjust the global
		 * flag indicating that the main database file has been
		 * modified (this will consequently update the file's ctime, mtime, and size)
		 */
		if(db_is_primary == true)
		{
			/* Changes have been made to the database. Update
			   this in the global variable value. */
			config->db_primary_file_modified = true;
		}
	}

	/* Cleanup */
	if(SUCCESS == status)
	{
		status = db_close(db,&db_file_modified);
	}

	provide(status);
}
