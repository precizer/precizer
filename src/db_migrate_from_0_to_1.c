/**
 * @file db_migrate_from_0_to_1.c
 * @brief
 */

#include "precizer.h"

/**
 * @brief Process single row from SQLite result
 * @param stmt Prepared statement with current row
 * @return Operation status
 */
static Return process_row(sqlite3_stmt *stmt){
	Return status = SUCCESS;
	const struct stat *stat = {0};

	/* Allocate memory for new blob data */
	CmpctStat new_stat = {0};

	/* Get blob data from the 'stat' column (column index 4) */
	stat = sqlite3_column_blob(stmt,4);

	if(NULL == stat)
	{
		slog(ERROR,"NULL blob data\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Copying essential elements from the old structure to the new one */
		status = stat_copy(stat,&new_stat);
	}

	if(SUCCESS == status)
	{
		/* Get row ID for update */
		sqlite3_int64 row_id = sqlite3_column_int64(stmt,0);

		/* Prepare update statement */
		sqlite3_stmt *update_stmt = NULL;
		const char *update_sql = "UPDATE files SET stat = ? WHERE ID = ?";

		if(SQLITE_OK != sqlite3_prepare_v2(sqlite3_db_handle(stmt),update_sql,-1,&update_stmt,NULL))
		{
			slog(ERROR,"Error preparing update statement: %s\n",sqlite3_errmsg(sqlite3_db_handle(stmt)));
			status = FAILURE;
		} else {
			/* Bind parameters */
			if(SQLITE_OK != sqlite3_bind_blob(update_stmt,1,&new_stat,sizeof(CmpctStat),SQLITE_STATIC))
			{
				slog(ERROR,"Error binding blob parameter\n");
				status = FAILURE;
			} else if(SQLITE_OK != sqlite3_bind_int64(update_stmt,2,row_id)){
				slog(ERROR,"Error binding row ID parameter\n");
				status = FAILURE;
			} else if(SQLITE_DONE != sqlite3_step(update_stmt)){
				slog(ERROR,"Error executing update statement\n");
				status = FAILURE;
			} else {
				/* Changes have been made to the database. Update
				   this in the global variable value. */
				config->something_has_been_changed = true;
			}

			sqlite3_finalize(update_stmt);
		}
	}

	return(status);
}

/**
 * @brief Process all rows in the database
 * @param db_path Path to SQLite database file
 * @return Operation status
 */
static Return process_database(sqlite3 *db){
	Return status = SUCCESS;
	sqlite3_stmt *stmt = NULL;

	const char *select_sql = "SELECT * FROM files";

	if(SQLITE_OK != sqlite3_prepare_v2(db,select_sql,-1,&stmt,NULL))
	{
		slog(ERROR,"Error preparing statement: %s\n",sqlite3_errmsg(db));
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

			status = process_row(stmt);

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

	return(status);
}

/**
 * @brief Migrates database from version 0 to version 1
 * @param db_file_path Path to the SQLite database file
 * @return Return status code
 */
Return migrate_from_0_to_1(const char *db_file_path){
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	char *err_msg = NULL;

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Database migration is not required\n");
		return(status);
	}

	/* Open database in safe mode */
	int rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Failed to open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Set safety pragmas */
		const char *pragmas = "PRAGMA journal_mode=WAL;"
		           "PRAGMA strict = ON;"
		           "PRAGMA synchronous=FULL;"
		           "PRAGMA foreign_keys=ON;"
		           "PRAGMA locking_mode=EXCLUSIVE;";

		rc = sqlite3_exec(db,pragmas,NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to set pragmas: %s\n",err_msg);
			sqlite3_free(err_msg);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Begin transaction */
		rc = sqlite3_exec(db,"BEGIN TRANSACTION",NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to begin transaction: %s\n",err_msg);
			sqlite3_free(err_msg);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Perform create table query */
		rc = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS metadata (db_version INTEGER NOT NULL UNIQUE)",NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to create table: %s\n",err_msg);
			sqlite3_free(err_msg);
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Perform version update query */
		status = process_database(db);

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
				slog(ERROR,"Failed to rollback transaction: %s\n",err_msg);
				status = FAILURE;
			}

		} else {
			/* Commit transaction */
			rc = sqlite3_exec(db,"COMMIT",NULL,NULL,&err_msg);

			if(SQLITE_OK != rc)
			{
				slog(ERROR,"Failed to commit transaction: %s\n",err_msg);
				sqlite3_free(err_msg);
				status = FAILURE;

				/* Attempt rollback */
				sqlite3_exec(db,"ROLLBACK",NULL,NULL,NULL);
			}
		}
	}

	/* Cleanup */
	if(db != NULL)
	{
		sqlite3_close(db);
	}

	return(status);
}
