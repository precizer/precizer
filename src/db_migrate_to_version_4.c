/**
 * @file db_migrate_to_version_4.c
 * @brief Migration to database version 4
 *
 * This legacy can be removed in 2036 (10-year Long-Term Support)
 */

#include "precizer.h"
#include "db_upgrade.h"

/**
 * @brief Convert CmpctStat_v1 into CmpctStat (v4 layout).
 *
 * Caller may pass a zero-initialized source for corrupted rows; in that case
 * the destination remains a valid zeroed v4 record.
 */
static void convert_blob_to_v4_stat(
	const CmpctStat_v1 *source,
	CmpctStat          *destination)
{
	memset(destination,0,sizeof(*destination));

	destination->st_size = source->st_size;
	destination->st_blocks = BLKCNT_UNKNOWN;
	destination->st_dev = 0;
	destination->st_ino = 0;
	destination->mtim_tv_sec = source->mtim_tv_sec;
	destination->mtim_tv_nsec = source->mtim_tv_nsec;
	destination->ctim_tv_sec = source->ctim_tv_sec;
	destination->ctim_tv_nsec = source->ctim_tv_nsec;
}

/**
 * @brief Migrate one files row to v4 stat format.
 *
 * Row with invalid blob size/content is not fatal: a zeroed source is used and
 * a zeroed v4 stat is written. FAILURE is returned only for SQLite errors.
 */
static Return process_row(
	sqlite3_stmt *stmt,
	bool         *db_file_modified)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int rc = SQLITE_OK;
	int blob_size = 0;

	sqlite3_int64 row_id = sqlite3_column_int64(stmt,0);
	CmpctStat_v1 zero_source = {0};
	const CmpctStat_v1 *source = &zero_source;

	blob_size = sqlite3_column_bytes(stmt,1);

	if(blob_size == (int)sizeof(CmpctStat_v1))
	{
		const CmpctStat_v1 *blob = sqlite3_column_blob(stmt,1);

		if(blob != NULL)
		{
			source = blob;
		} else {
			slog(ERROR,"Invalid v3 stat blob pointer for row id=%lld (size=%d). Zero stat will be stored\n",(long long)row_id,blob_size);
		}
	} else {
		slog(ERROR,"Invalid v3 stat blob size for row id=%lld (got=%d, expected=%zu). Zero stat will be stored\n",(long long)row_id,blob_size,sizeof(CmpctStat_v1));
	}

	CmpctStat destination = {0};
	convert_blob_to_v4_stat(source,&destination);

	sqlite3_stmt *update_stmt = NULL;
	const char *update_sql = "UPDATE files SET stat = ?1 WHERE ID = ?2";

	rc = sqlite3_prepare_v2(sqlite3_db_handle(stmt),update_sql,-1,&update_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error preparing update statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_blob(update_stmt,1,&destination,sizeof(CmpctStat),SQLITE_STATIC);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error binding stat blob");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(update_stmt,2,row_id);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error binding row id");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_step(update_stmt);

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error executing update statement");
			status = FAILURE;
		} else {
			*db_file_modified = true;
		}
	}

	sqlite3_finalize(update_stmt);

	provide(status);
}

/**
 * @brief Convert all files.stat blobs to v4 format.
 *
 * Corrupted row payloads do not stop migration; iteration stops only on SQLite
 * errors or external interruption.
 */
static Return process_database(
	sqlite3 *db,
	bool    *db_file_modified)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_OK;

	const char *select_sql = "SELECT ID, stat FROM files";

	rc = sqlite3_prepare_v2(db,select_sql,-1,&stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Error preparing statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		while(SQLITE_ROW == (rc = sqlite3_step(stmt)))
		{
			if(global_interrupt_flag == true)
			{
				break;
			}

			status = process_row(stmt,db_file_modified);

			if(SUCCESS != status)
			{
				break;
			}
		}

		if(SUCCESS == status && global_interrupt_flag == false)
		{
			if(SQLITE_DONE != rc)
			{
				log_sqlite_error(db,rc,NULL,"Error iterating over rows during migration");
				status = FAILURE;
			}
		}
	}

	sqlite3_finalize(stmt);

	provide(status);
}

/**
 * @brief Normalize journaling and flush WAL artifacts before migration.
 *
 * Switches journal mode to DELETE, verifies returned mode value, and runs
 * wal_checkpoint(TRUNCATE).
 */
static Return normalize_journal_mode(sqlite3 *db)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_OK;

	rc = sqlite3_prepare_v2(db,"PRAGMA journal_mode=DELETE;",-1,&stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to prepare journal_mode switch");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_step(stmt);

		if(SQLITE_ROW == rc)
		{
			const unsigned char *mode = sqlite3_column_text(stmt,0);

			if(mode == NULL || strcmp((const char *)mode,"delete") != 0)
			{
				slog(ERROR,"journal_mode switch failed, current mode: %s\n",mode ? (const char *)mode : "(null)");
				status = FAILURE;
			}
		} else {
			log_sqlite_error(db,rc,NULL,"journal_mode switch did not return a row");
			status = FAILURE;
		}
	}

	sqlite3_finalize(stmt);

	if(SUCCESS == status)
	{
		rc = sqlite3_exec(db,"PRAGMA wal_checkpoint(TRUNCATE);",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,NULL,"Failed to checkpoint WAL");
			status = FAILURE;
		}
	}

	provide(status);
}

/**
 * @brief Migrate database schema/data to version 4.
 *
 * Migration runs inside an explicit transaction and issues ROLLBACK on any
 * FAILURE after BEGIN TRANSACTION.
 *
 * @param[in] db_file_path Path to the SQLite database file.
 * @return Return status code.
 */
Return db_migrate_to_version_4(const char *db_file_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	char *err_msg = NULL;
	bool db_file_modified = false;
	bool transaction_started = false;
	int rc = SQLITE_OK;

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Database migration is not required\n");
		provide(status);
	}

	rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const char *pragmas =
		        "PRAGMA strict=ON;"
		        "PRAGMA fsync=ON;"
		        "PRAGMA synchronous=EXTRA;"
		        "PRAGMA locking_mode=EXCLUSIVE;";

		rc = sqlite3_exec(db,pragmas,NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,err_msg,"Failed to set pragmas");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = normalize_journal_mode(db);
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_exec(db,"BEGIN TRANSACTION",NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,err_msg,"Failed to begin transaction");
			status = FAILURE;
		} else {
			transaction_started = true;
		}
	}

	if(SUCCESS == status)
	{
		status = process_database(db,&db_file_modified);

		if(SUCCESS != status)
		{
			slog(ERROR,"Database processing failed\n");
		}
	}

	if(transaction_started == true)
	{
		if(global_interrupt_flag == true)
		{
			rc = sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);

			if(SQLITE_OK == rc)
			{
				slog(TRACE,"The transaction has been rolled back\n");

				if(SUCCESS == status)
				{
					status = WARNING;
				}
			} else {
				log_sqlite_error(db,rc,NULL,"Failed to rollback transaction");
				status = FAILURE;
			}
		} else if(SUCCESS != status){
			rc = sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);

			if(SQLITE_OK == rc)
			{
				slog(TRACE,"The transaction has been rolled back\n");
			} else {
				log_sqlite_error(db,rc,NULL,"Failed to rollback transaction");
				status = FAILURE;
			}
		} else {
			rc = sqlite3_exec(db,"COMMIT",NULL,NULL,&err_msg);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,err_msg,"Failed to commit transaction");
				status = FAILURE;
				sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);
			}
		}
	}

	if(SUCCESS == status && db_file_modified == true)
	{
		if(strcmp(db_file_path,confstr(db_primary_file_path)) == 0)
		{
			config->db_primary_file_modified = true;
		}
	}

	call(db_close(db,&config->db_primary_file_modified));

	provide(status);
}
