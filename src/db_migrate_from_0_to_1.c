/**
 * @file db_migrate_from_0_to_1.c
 * @brief
 */

#include "precizer.h"

#define STAT64_SIZE 144
#define STAT64_ST_SIZE_OFF 48
#define STAT64_ST_MTIM_OFF 88
#define STAT64_ST_CTIM_OFF 104

static Return cmpct_stat_is_sane(const CmpctStat *stat)
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

	if(stat->st_size < 0)
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

static Return populate_from_glibc_stat_blob(
	const void   *blob,
	const int    blob_size,
	CmpctStat    *new_stat)
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
 * @brief Process single row from SQLite result
 * @param stmt Prepared statement with current row
 * @return Operation status
 */
static Return process_row(
	sqlite3_stmt *stmt,
	bool         *db_file_modified)
{
	Return status = SUCCESS;
	const struct stat *stat = {0};
	int rc = SQLITE_OK;

	/* Allocate memory for new blob data */
	CmpctStat new_stat = {0};

	/* Get blob data from the 'stat' column (column index 4) */
	stat = sqlite3_column_blob(stmt,4);
	int blob_size = sqlite3_column_bytes(stmt,4);

	if(NULL == stat)
	{
		slog(ERROR,"NULL blob data\n");
		status = FAILURE;
	}

	run(populate_from_glibc_stat_blob(stat,blob_size,&new_stat));

	if(SUCCESS != status)
	{
		slog(ERROR,"Failed to convert legacy stat blob (len=%d) into compact stat\n",blob_size);
	}

	if(SUCCESS == status)
	{
		/* Get row ID for update */
		sqlite3_int64 row_id = sqlite3_column_int64(stmt,0);

		/* Prepare update statement */
		sqlite3_stmt *update_stmt = NULL;
		const char *update_sql = "UPDATE files SET stat = ? WHERE ID = ?";

		rc = sqlite3_prepare_v2(sqlite3_db_handle(stmt),update_sql,-1,&update_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(sqlite3_db_handle(stmt),rc,NULL,"Error preparing update statement");
			status = FAILURE;
		} else {
			/* Bind parameters */
			rc = sqlite3_bind_blob(update_stmt,1,&new_stat,sizeof(CmpctStat),SQLITE_STATIC);

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
				/* Changes have been made to the database. Update
				   this in the global variable value. */
				*db_file_modified = true;
			}

			sqlite3_finalize(update_stmt);
		}
	}

	provide(status);
}

/**
 * @brief Process all rows in the database
 * @param db_path Path to SQLite database file
 * @return Operation status
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
		while(SQLITE_ROW == sqlite3_step(stmt))
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
	}

	/* Cleanup */
	if(NULL != stmt)
	{
		sqlite3_finalize(stmt);
	}

	provide(status);
}

/**
 * @brief Migrates database from version 0 to version 1
 * @param db_file_path Path to the SQLite database file
 * @return Return status code
 */
Return db_migrate_from_0_to_1(const char *db_file_path)
{
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	char *err_msg = NULL;
	bool db_file_modified = false;
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

	if(SUCCESS == status)
	{
		if(global_interrupt_flag == true)
		{
			/* Attempt rollback */
			rc = sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);

			if(SQLITE_OK == rc)
			{
				slog(TRACE,"The transaction has been rolled back\n");
				status = WARNING;
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
