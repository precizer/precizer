#include "precizer.h"

/**
 * @brief Start a batch before the main traversal saves a file record
 *
 * The batch uses the primary connection settings established by db_init().
 * Later saves join the active batch without moving its deadline. Dry-run
 * does not start transactions. Outside dry-run, the caller supplies a writable
 * connection
 *
 * @return SUCCESS when a batch is ready or unnecessary, otherwise FAILURE
 */
Return db_file_transaction_begin(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(config->dry_run == true)
	{
		provide(status);
	}

	/*
	 * Reuse the current batch without starting another transaction or resetting
	 * its timer. Autocommit must remain disabled while the batch is active.
	 * If SQLite has ended the transaction (for example, after an automatic
	 * rollback), report failure to prevent further writes outside the batch
	 */
	if(config->file_transaction_active == true)
	{
		if(sqlite3_get_autocommit(config->db) != 0)
		{
			slog(ERROR,"File traversal transaction ended before commit\n");
			status = FAILURE;
		}

		provide(status);
	}

	/* Use the primary connection settings established by db_init() */
	const int rc = sqlite3_exec(config->db,"BEGIN IMMEDIATE;",NULL,NULL,NULL);

	if(rc != SQLITE_OK)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to begin file traversal transaction");
		status = FAILURE;
	} else {
		config->file_transaction_active = true;
		config->file_transaction_started_ns = cur_time_monotonic_ns();
	}

	provide(status);
}

/**
 * @brief Commit the current batch of file records, if one is active
 * @return SUCCESS after commit or when no batch is active, otherwise FAILURE
 */
Return db_file_transaction_commit(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(config->file_transaction_active == true)
	{
		const int rc = sqlite3_exec(config->db,"COMMIT;",NULL,NULL,NULL);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to commit file traversal transaction");
			status = FAILURE;
		} else {
			config->file_transaction_active = false;
		}
	}

	provide(status);
}

/**
 * @brief Commit pending file records when their shared interval has elapsed
 * @param now_ns Current monotonic time in nanoseconds
 * @return SUCCESS when the deadline was handled, otherwise FAILURE
 */
Return db_file_transaction_check(const long long int now_ns)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(config->file_transaction_active == true
	        && now_ns - config->file_transaction_started_ns >= DB_CHECKPOINT_INTERVAL_NS)
	{
		call(db_file_transaction_commit());
	}

	provide(status);
}

/**
 * @brief Roll back any pending main-traversal transaction
 *
 * Does nothing when no file transaction is active. If SQLite already rolled
 * back automatically, only the traversal transaction flag is cleared
 *
 * Database operations after traversal use the same connection settings
 *
 * @return Cleanup status, to be combined with the traversal result by the caller
 */
Return db_file_transaction_rollback(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(config->file_transaction_active == true)
	{
		/* SQLite may have rolled back automatically after an I/O or storage error */
		if(sqlite3_get_autocommit(config->db) == 0)
		{
			const int rc = sqlite3_exec(config->db,"ROLLBACK;",NULL,NULL,NULL);

			if(rc != SQLITE_OK)
			{
				log_sqlite_error(config->db,rc,NULL,"Failed to roll back file traversal transaction");
				status = FAILURE;
			}
		}

		/* Clear the flag after a successful rollback or if SQLite had already
		   ended the transaction. Leave it set on failure because the transaction's
		   state is uncertain */
		if(SUCCESS & status)
		{
			config->file_transaction_active = false;
		}
	}

	provide(status);
}
