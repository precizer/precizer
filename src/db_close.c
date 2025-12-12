#include "precizer.h"

/**
 * Disable journaling, flush the journal to the main database,
 * clear the cache, and close the database
 *
 */
Return db_close(
	sqlite3    *db,
	const bool *db_file_modified)
{
	Return status = SUCCESS;
	int rc = SQLITE_OK;

	/* Cleanup and close previously used DB */
	if(db != NULL)
	{
		if(*db_file_modified == true)
		{
			sqlite3_stmt *stmt;

			while((stmt = sqlite3_next_stmt(db,NULL)) != NULL)
			{
				slog(ERROR,"Attention! The program is in the process of shutting down, but there are still uncompleted SQLite statements!\n");
				sqlite3_finalize(stmt);
			}

			/**
			 * @brief Configure SQLite for maximum reliability using PRAGMA
			 * @note This is the second approach to ensure data integrity
			 * @details Sets synchronous mode to FULL for maximum durability
			 */
			const char *sql =
			        "PRAGMA journal_mode=DELETE;"
			        "PRAGMA fsync=ON;"
			        "PRAGMA synchronous=EXTRA;"
			        "PRAGMA locking_mode=EXCLUSIVE;";

			rc = sqlite3_exec(db,sql,NULL,NULL,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,NULL,"Warning: failed to tune database integrity");
				status = FAILURE;
			}
		}

		/**
		 * @brief Force cache flush to disk for data persistence
		 * @note This is the first approach to ensure data integrity
		 */
		rc = sqlite3_db_cacheflush(db);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Warning: failed to flush database");
			status = FAILURE;
		}

		/**
		 * @brief Close database connection and cleanup resources
		 * @note Must be called to prevent resource leaks
		 */
		rc = sqlite3_close(db);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Warning: failed to close database");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		slog(TRACE,"The database connection has been closed\n");
	}

	provide(status);
}
