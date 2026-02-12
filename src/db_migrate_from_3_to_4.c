/**
 * @file db_migrate_from_3_to_4.c
 * @brief Migration to database version 4
 *
 * This legacy can be removed in 2036 (10-year Long-Term Support)
 */

#include "precizer.h"
#include "db_upgrade.h"

/**
 * @brief Convert v3 stat blob into v4 compact stat.
 */
static Return convert_blob_to_v4_stat(
	const CmpctStat_v1 *source,
	CmpctStat          *destination)
{
	if(source == NULL || destination == NULL)
	{
		return FAILURE;
	}

	memset(destination,0,sizeof(*destination));

	if(source->st_size < 0)
	{
		return FAILURE;
	}

	destination->st_size = source->st_size;
	destination->st_blocks = BLKCNT_UNKNOWN;
	destination->st_dev = 0;
	destination->st_ino = 0;
	destination->mtim_tv_sec = source->mtim_tv_sec;
	destination->mtim_tv_nsec = source->mtim_tv_nsec;
	destination->ctim_tv_sec = source->ctim_tv_sec;
	destination->ctim_tv_nsec = source->ctim_tv_nsec;

	return SUCCESS;
}

/**
 * @brief Migrate one row's stat blob to v4 format.
 */
static Return process_row(
	sqlite3_stmt *stmt,
	bool         *db_file_modified)
{
	Return status = SUCCESS;
	int rc = SQLITE_OK;
	int blob_size = 0;

	sqlite3_int64 row_id = sqlite3_column_int64(stmt,0);
	const CmpctStat_v1 *source = NULL;

	blob_size = sqlite3_column_bytes(stmt,1);

	if(blob_size < (int)sizeof(CmpctStat_v1))
	{
		slog(ERROR,
		     "Invalid v3 stat blob size for row id=%lld (got=%d, expected>=%zu)\n",
		     (long long)row_id,
		     blob_size,
		     sizeof(CmpctStat_v1));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		source = sqlite3_column_blob(stmt,1);
	}

	CmpctStat destination = {0};

	if(SUCCESS == status)
	{
		status = convert_blob_to_v4_stat(source,&destination);
	}

	if(status != SUCCESS)
	{
		slog(ERROR,"Failed to convert v3 stat blob for row id=%lld\n",(long long)row_id);
		provide(status);
	}

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

	if(update_stmt != NULL)
	{
		sqlite3_finalize(update_stmt);
	}

	provide(status);
}

/**
 * @brief Convert all stored stat blobs from v3 to v4 format.
 */
static Return process_database(
	sqlite3 *db,
	bool    *db_file_modified)
{
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

		if(SUCCESS == status && global_interrupt_flag == false && SQLITE_DONE != rc)
		{
			log_sqlite_error(db,rc,NULL,"Error iterating over rows during migration");
			status = FAILURE;
		}
	}

	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	provide(status);
}

Return db_migrate_from_3_to_4(const char *db_file_path)
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

	rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(db,rc,NULL,"Failed to open database");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
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
		rc = sqlite3_exec(db,"BEGIN TRANSACTION",NULL,NULL,&err_msg);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(db,rc,err_msg,"Failed to begin transaction");
			status = FAILURE;
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

	if(SUCCESS == status)
	{
		if(global_interrupt_flag == true)
		{
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
		if(strcmp(db_file_path,config->db_primary_file_path) == 0)
		{
			config->db_primary_file_modified = true;
		}
	}

	call(db_close(db,&config->db_primary_file_modified));

	provide(status);
}
