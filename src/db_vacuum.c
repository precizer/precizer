#include "precizer.h"

/**
 *
 * The VACUUM command rebuilds the database file,
 * repacking it into a minimal amount of disk space.
 *
 */
Return db_vacuum(const char *db_file_path){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	sqlite3 *db = NULL;
	char *err_msg = NULL;
	bool db_is_primary = false;

	/* Validate input parameters */
	if(db_file_path == NULL)
	{
		slog(ERROR,"Invalid input parameters: db_file_path\n");
		return(FAILURE);
	}

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. The database doesn't require vacuuming\n");
		return(status);
	}

	if(strcmp(config->db_file_path,db_file_path) == 0)
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
		const char *sql = "PRAGMA analyze;"
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
			if(db_is_primary == true)
			{
				slog(EVERY,"The primary database has been vacuumed\n");
			} else {
				slog(EVERY,"The database has been vacuumed\n");
			}
		}
	}

	/* Cleanup */
	if(db != NULL)
	{
		/**
		 * @brief Force cache flush to disk for data persistence
		 * @note This is the first approach to ensure data integrity
		 */
		if(SQLITE_OK != sqlite3_db_cacheflush(db))
		{
			slog(ERROR,"Warning: failed to flush database: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}

		/**
		 * @brief Configure SQLite for maximum reliability using PRAGMA
		 * @note This is the second approach to ensure data integrity
		 * @details Sets synchronous mode to FULL for maximum durability
		 */
		if(SQLITE_OK != sqlite3_exec(db,"PRAGMA synchronous = FULL;",NULL,NULL,NULL))
		{
			slog(ERROR,"Warning: failed to tune database integrity: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}

		/**
		 * @brief Close database connection and cleanup resources
		 * @note Must be called to prevent resource leaks
		 */
		if(SQLITE_OK != sqlite3_close(db))
		{
			slog(ERROR,"Warning: failed to close database: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	return(status);
}
