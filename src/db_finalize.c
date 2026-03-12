#include "precizer.h"

/**
 * @brief Finalize a prepared SQLite statement and reset its pointer
 *
 * Also performs WAL checkpoint and cache flush in the same way as db_close()
 * to ensure WAL/SHM files are cleaned up when the database was modified.
 *
 * @param[in] db Pointer to SQLite database handle
 * @param[in] db_alias Attached database alias name
 * @param[in,out] stmt Pointer to prepared statement pointer to finalize
 * @return Return status code
 */
Return db_finalize(
	sqlite3      *db,
	const char   *db_alias,
	sqlite3_stmt **stmt)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(db == NULL || db_alias == NULL || stmt == NULL)
	{
		slog(ERROR,"Invalid input parameters: db=%p, db_alias=%p, stmt=%p\n",db,db_alias,stmt);
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(*stmt != NULL)
		{
			int rc = sqlite3_finalize(*stmt);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,NULL,"Failed to finalize SQLite statement");
				status = FAILURE;
			} else {
				*stmt = NULL;
			}
		}

		sqlite3_stmt *active_stmt = NULL;

		while((active_stmt = sqlite3_next_stmt(db,NULL)) != NULL)
		{
			slog(ERROR,"Attention! The program is in the process of shutting down, but there are still uncompleted SQLite statements!\n");
			sqlite3_finalize(active_stmt);
		}

		char *sql = NULL;

		if(asprintf(&sql,
			"PRAGMA %s.journal_mode=DELETE;"
			"PRAGMA %s.fsync=ON;"
			"PRAGMA %s.synchronous=EXTRA;"
			"PRAGMA %s.locking_mode=EXCLUSIVE;",
			db_alias,
			db_alias,
			db_alias,
			db_alias) == -1)
		{
			status = FAILURE;
			report("Memory allocation failed for WAL checkpoint SQL");
		}

		if(SUCCESS == status)
		{
			int rc = sqlite3_exec(db,sql,NULL,NULL,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,NULL,"Warning: failed to tune database integrity");
				status = FAILURE;
			}
		}

		free(sql);

		if(SUCCESS == status)
		{
			int rc = sqlite3_db_cacheflush(db);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,NULL,"Warning: failed to flush database");
				status = FAILURE;
			}
		}
	}

	provide(status);
}
