/**
 * @file db_save_prefixes.c
 * @brief Database operations for directory prefix paths
 */

#include "precizer.h"

/**
 * @brief Return the number of rows changed by the current SQLite connection
 *
 * SQLite 3.37.0 added `sqlite3_total_changes64()`. Older system SQLite
 * packages only provide `sqlite3_total_changes()`, so dynamic builds use the
 * older 32-bit counter and widen its result to keep the local return type stable.
 * This legacy can be removed in 2036 (10-year Long-Term Support)
 * Replacement: `db_retrieve_total_changes()` -> `sqlite3_total_changes64()`
 *
 * @param[in] db SQLite database connection
 * @return Total number of changed rows reported by SQLite for this connection
 */
static sqlite3_int64 db_retrieve_total_changes(sqlite3 *db)
{
#if SQLITE_VERSION_NUMBER >= 3037000
	return(sqlite3_total_changes64(db));
#else
	return((sqlite3_int64)sqlite3_total_changes(db));
#endif
}

/**
 * @brief Save the current traversal roots into the `paths` table
 *
 * The positional directories accepted by normal scanning mode are stored in
 * `config->roots`. This function writes each root exactly as it was accepted
 * from the command line, so the database keeps the user's chosen root spelling
 * while file records stay relative to that root
 *
 * In `--compare` mode the function returns immediately because compare
 * arguments are database files
 *
 * Prefix rows are written in one transaction whenever the selected mode allows
 * database changes. With `--force`, obsolete path rows are removed before the
 * current roots are inserted. If an SQLite operation fails, the transaction is
 * rolled back. With `--dry-run` against an already existing physical database,
 * inserts are skipped so the on-disk database is not modified
 *
 * The primary database is marked as modified only when at least one prefix row
 * changes and any required transaction commits successfully
 *
 * For example, after parsing `precizer --database tree.db /home/me/tree`,
 * `config->roots` contains `/home/me/tree`, and this function ensures that the
 * prefix exists in the database
 *
 * @return `SUCCESS` when all required prefixes are present or intentionally
 *         skipped by mode. `FAILURE` when an SQLite operation fails
 */
Return db_save_prefixes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Result code returned by the most recent SQLite operation */
	int rc = SQLITE_OK;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	/* Skip in comparison mode */
	if(config->compare == true)
	{
		provide(status);
	}

	/*
	 * Dry-run mode has two database scenarios.
	 * If a physical primary database already exists, it is opened read-only and
	 * must not be changed, so prefix saving stops here.
	 * If no physical database exists, dry-run uses an in-memory SQLite database.
	 * That temporary database still needs path prefixes so the simulated scan
	 * behaves like a normal run
	 */
	if(config->dry_run == true && config->db_primary_file_exists == true)
	{
		provide(status);
	}

	/*
	 * Remember how many rows this database connection has changed so far. After
	 * the transaction, a larger value means that prefix rows were actually changed
	 */
	const sqlite3_int64 total_changes_before = db_retrieve_total_changes(config->db);

	/*
	 * Start one transaction for the complete prefix update. This keeps removals
	 * and additions together so a later error can roll back the whole change
	 */
	rc = sqlite3_exec(config->db,"BEGIN TRANSACTION",NULL,NULL,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to begin prefix update transaction");
		provide(FAILURE);
	}

	/*
	 * Update the paths table inside the transaction. Force mode first removes
	 * obsolete prefixes, then the remaining modes add traversal roots that are
	 * not already present
	 */
	if(config->force == true && config->dry_run == false)
	{
		/* Prepared statement used to delete obsolete prefix rows */
		sqlite3_stmt *delete_stmt = NULL;

		/* Query that removes prefix rows which are no longer needed */
		const char *delete_sql = "DELETE FROM paths WHERE ID IN (SELECT path_id FROM the_path_id_does_not_exists);";

		if(SUCCESS == status)
		{
			rc = sqlite3_prepare_v2(config->db,delete_sql,-1,&delete_stmt,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Can't prepare delete statement");
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{
			/* Execute SQL statement */
			rc = sqlite3_step(delete_stmt);

			if(SQLITE_DONE != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Delete statement didn't return DONE");
				status = FAILURE;
			}
		}

		rc = sqlite3_finalize(delete_stmt);

		if(SUCCESS == status && SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to finalize delete statement");
			status = FAILURE;
		}
	}

	/*
	 * Insert every configured traversal root that is not already present.
	 * Read-only dry-run scans return before the transaction starts. Each loop
	 * iteration exposes the current root descriptor as `root`
	 */
	m_string_array_foreach(conf(roots),root)
	{
		/* Read-only text view of the current root descriptor */
		const char *root_path = m_text(root);

		/* Number of bytes in the current root path, excluding its terminator */
		size_t root_path_length;

		if(SUCCESS == status)
		{
			status = m_string_length(root,&root_path_length);
		}

		/* Query that inserts the current prefix unless its unique value already exists */
		const char *insert_sql = "INSERT OR IGNORE INTO paths(prefix) VALUES(?1);";

		/* Prepared statement for the current insert attempt */
		sqlite3_stmt *insert_stmt = NULL;

		/* Existing prefixes are ignored by the database constraint */
		if(SUCCESS == status)
		{
			/* Create SQL statement. Prepare to write */
			rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Can't prepare insert statement %s",insert_sql);
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{
			rc = sqlite3_bind_text(insert_stmt,1,root_path,(int)root_path_length,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Error binding value in insert");
				status = FAILURE;
			}
		}

		/* Execute SQL statement */
		if(SUCCESS == status)
		{
			rc = sqlite3_step(insert_stmt);

			if(SQLITE_DONE != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Insert statement didn't return DONE");
				status = FAILURE;
			}
		}

		rc = sqlite3_finalize(insert_stmt);

		if(SUCCESS == status && SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to finalize insert statement");
			status = FAILURE;
		}

		if(SUCCESS != status)
		{
			break;
		}
	}

	/*
	 * Finish the transaction after all prefix operations. Commit the complete
	 * update after success, or roll it back if any operation failed
	 */
	if(SUCCESS == status)
	{
		/* Commit transaction */
		rc = sqlite3_exec(config->db,"COMMIT",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to commit prefix update transaction");
			status = FAILURE;
		}
	}

	/*
	 * Roll back after an error only while the transaction is still open. SQLite
	 * may already have rolled it back automatically for some failures
	 */
	if(SUCCESS != status && sqlite3_get_autocommit(config->db) == 0)
	{
		/* Attempt rollback */
		rc = sqlite3_exec(config->db,"ROLLBACK",NULL,NULL,NULL);

		if(SQLITE_OK == rc)
		{
			slog(TRACE,"The prefix update transaction has been rolled back\n");
		} else {
			log_sqlite_error(config->db,rc,NULL,"Failed to rollback prefix update transaction");
			status = FAILURE;
		}
	}

	if(SUCCESS == status
	        && config->dry_run == false
	        && total_changes_before < db_retrieve_total_changes(config->db))
	{
		/*
		 * In-memory dry-run prefix writes are only simulation data.
		 * They must not mark the real primary database as modified
		 */
		/* Reflect changes in global */
		config->db_primary_file_modified = true;
	}

	provide(status);
}
