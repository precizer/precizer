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
			        "PRAGMA fsync=ON;"
			        "PRAGMA synchronous=EXTRA;"
			        "PRAGMA locking_mode=EXCLUSIVE;"
			        "PRAGMA wal_checkpoint(TRUNCATE);";

			if(SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,NULL))
			{
				slog(ERROR,"Warning: failed to tune database integrity: %s\n",sqlite3_errmsg(db));
				status = FAILURE;
			}
		}

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
		 * @brief Close database connection and cleanup resources
		 * @note Must be called to prevent resource leaks
		 */
		if(SQLITE_OK != sqlite3_close(db))
		{
			slog(ERROR,"Warning: failed to close database: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		slog(TRACE,"The connection to the primary database has been closed\n",sqlite3_errmsg(db));
	}

	provide(status);
}
