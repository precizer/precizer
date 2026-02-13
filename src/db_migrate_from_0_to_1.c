/**
 * @file db_migrate_from_0_to_1.c
 * @brief Migration to database version 1
 *
 * This legacy can be removed in 2034 (10-year Long-Term Support)
 */

#include "precizer.h"
#include "db_upgrade.h"

#define STAT64_SIZE 144
#define STAT64_ST_SIZE_OFF 48
#define STAT64_ST_MTIM_OFF 88
#define STAT64_ST_CTIM_OFF 104

/**
 * @brief Validate sanity of a compact stat structure.
 *
 * Checks timestamp ranges (0..999999999 for nsec, reasonable sec).
 *
 * @param stat Pointer to compact stat to validate.
 * @return SUCCESS if fields look sane, FAILURE otherwise.
 */
static Return cmpct_stat_is_sane(const CmpctStat_v1 *stat)
{
	if(NULL == stat)
	{
		return(FAILURE);
	}

	if(stat->mtim_tv_nsec < 0 || stat->mtim_tv_nsec > 999999999)
	{
		return(FAILURE);
	}

	if(stat->ctim_tv_nsec < 0 || stat->ctim_tv_nsec > 999999999)
	{
		return(FAILURE);
	}

	const time_t max_ts = (time_t)32503680000; // Year 3000 upper bound

	if(stat->mtim_tv_sec < 0 || stat->mtim_tv_sec > max_ts)
	{
		return(FAILURE);
	}

	if(stat->ctim_tv_sec < 0 || stat->ctim_tv_sec > max_ts)
	{
		return(FAILURE);
	}

	return(SUCCESS);
}

/**
 * @brief Convert a glibc/Linux stat blob into a compact stat.
 *
 * The blob format corresponds to 64-bit glibc stat layout (144 bytes) used in legacy DB v0.
 * Extracts st_size, st_mtim, st_ctim by fixed offsets and fills CmpctStat_v1.
 *
 * @param blob Pointer to raw blob data.
 * @param blob_size Size of the blob in bytes.
 * @param new_stat Output compact stat structure.
 * @return SUCCESS on successful conversion and sanity check, FAILURE otherwise.
 */
static Return populate_from_glibc_stat_blob(
	const void   *blob,
	const int    blob_size,
	CmpctStat_v1 *new_stat)
{
	if(blob_size < STAT64_SIZE)
	{
		return(FAILURE);
	}

	const unsigned char *b = (const unsigned char *)blob;
	int64_t st_size = 0;
	int64_t mtim_sec = 0;
	int64_t mtim_nsec = 0;
	int64_t ctim_sec = 0;
	int64_t ctim_nsec = 0;

	memcpy(&st_size,b + STAT64_ST_SIZE_OFF,sizeof(st_size));
	memcpy(&mtim_sec,b + STAT64_ST_MTIM_OFF,sizeof(mtim_sec));
	memcpy(&mtim_nsec,b + STAT64_ST_MTIM_OFF + sizeof(int64_t),sizeof(mtim_nsec));
	memcpy(&ctim_sec,b + STAT64_ST_CTIM_OFF,sizeof(ctim_sec));
	memcpy(&ctim_nsec,b + STAT64_ST_CTIM_OFF + sizeof(int64_t),sizeof(ctim_nsec));

	new_stat->st_size = (off_t)st_size;
	new_stat->mtim_tv_sec = (time_t)mtim_sec;
	new_stat->mtim_tv_nsec = (long)mtim_nsec;
	new_stat->ctim_tv_sec = (time_t)ctim_sec;
	new_stat->ctim_tv_nsec = (long)ctim_nsec;

	return(cmpct_stat_is_sane(new_stat));
}

/**
 * @brief Convert and rewrite one row of the files table.
 *
 * Invalid legacy blobs are replaced with a zeroed CmpctStat_v1 and migration
 * continues; FAILURE is returned only for SQLite errors.
 *
 * @param[in] stmt Prepared statement positioned at the current row.
 * @param[out] db_file_modified Set to true when row update succeeds.
 * @return Operation status.
 */
static Return process_row(
	sqlite3_stmt *stmt,
	bool         *db_file_modified)
{
	Return status = SUCCESS;

	int rc = SQLITE_OK;

	sqlite3_int64 row_id = sqlite3_column_int64(stmt,0);

	/* Allocate memory for new blob data */
	CmpctStat_v1 new_stat = {0};

	/* Get blob data from the 'stat' column (column index 4) */
	const void *blob = sqlite3_column_blob(stmt,4);

	int blob_size = sqlite3_column_bytes(stmt,4);

	Return conversion_status = FAILURE;

	if(blob != NULL)
	{
		conversion_status = populate_from_glibc_stat_blob(blob,blob_size,&new_stat);
	}

	if(SUCCESS != conversion_status)
	{
		memset(&new_stat,0,sizeof(new_stat));
		slog(ERROR,"Invalid legacy stat blob for row id=%lld (len=%d). Zero v1 stat will be stored\n",(long long)row_id,blob_size);
	}

	/* Prepare update statement */
	sqlite3_stmt *update_stmt = NULL;
	const char *update_sql = "UPDATE files SET stat = ? WHERE ID = ?";

	rc = sqlite3_prepare_v2(sqlite3_db_handle(stmt),update_sql,-1,&update_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error preparing update statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Bind parameters */
		rc = sqlite3_bind_blob(update_stmt,1,&new_stat,sizeof(CmpctStat_v1),SQLITE_STATIC);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error binding blob parameter");
			status = FAILURE;
		} else if(SQLITE_OK != (rc = sqlite3_bind_int64(update_stmt,2,row_id))){
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error binding row ID parameter");
			status = FAILURE;
		} else if(SQLITE_DONE != (rc = sqlite3_step(update_stmt))){
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error executing update statement");
			status = FAILURE;
		} else {
			/* Changes have been made to the database.
			   Reflect this in the caller-provided flag. */
			*db_file_modified = true;
		}
	}

	if(update_stmt != NULL)
	{
		sqlite3_finalize(update_stmt);
	}

	provide(status);
}

/**
 * @brief Process all rows of the files table for v0->v1 conversion.
 *
 * Row-level data corruption does not stop migration. The function fails only on
 * SQLite iteration/update errors or external interruption.
 *
 * @param[in] db Open SQLite handle.
 * @param[out] db_file_modified Set to true when at least one row is updated.
 * @return Operation status.
 */
static Return process_database(
	sqlite3 *db,
	bool    *db_file_modified)
{
	Return status = SUCCESS;
	sqlite3_stmt *stmt = NULL;
	int rc = SQLITE_OK;

	const char *select_sql = "SELECT * FROM files";

	rc = sqlite3_prepare_v2(db,select_sql,-1,&stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Error preparing statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Process each row */
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

	/* Cleanup */
	if(NULL != stmt)
	{
		sqlite3_finalize(stmt);
	}

	provide(status);
}

/**
 * @brief Migrate database schema/data from version 0 to version 1.
 *
 * Migration runs inside an explicit transaction and issues ROLLBACK on any
 * FAILURE after BEGIN TRANSACTION.
 *
 * @param[in] db_file_path Path to the SQLite database file.
 * @return Return status code.
 */
Return db_migrate_from_0_to_1(const char *db_file_path)
{
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

	/* Open database in safe mode */
	rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Set safety pragmas */
		const char *pragmas =
		        "PRAGMA journal_mode=DELETE;"
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
		/* Begin transaction */
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
		/* Perform create table query */
		rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS metadata (db_version INTEGER NOT NULL UNIQUE)",NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,err_msg,"Failed to create table");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Perform version update query */
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
			/* Attempt rollback */
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
			/* Attempt rollback */
			rc = sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);

			if(SQLITE_OK == rc)
			{
				slog(TRACE,"The transaction has been rolled back\n");
			} else {
				log_sqlite_error(db,rc,NULL,"Failed to rollback transaction");
				status = FAILURE;
			}
		} else {
			/* Commit transaction */
			rc = sqlite3_exec(db,"COMMIT",NULL,NULL,&err_msg);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(db,rc,err_msg,"Failed to commit transaction");
				status = FAILURE;

				/* Attempt rollback */
				sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);
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
		if(db_file_modified == true)
		{
			if(strcmp(db_file_path,config->db_primary_file_path) == 0)
			{
				/* Changes have been made to the database. Update
				   this in the global variable value. */
				config->db_primary_file_modified = true;
			}
		}
	}

	/* Cleanup */
	call(db_close(db,&config->db_primary_file_modified));

	provide(status);
}
