/**
 * @file db_migrate_from_2_to_3.c
 * @brief Migration to database version 3
 */

#include "precizer.h"

/**
 * @brief Migrates database from version 2 to version 3
 * @details Forces journal_mode to DELETE to disable WAL and cleans up WAL files.
 *          No schema changes are applied; this migration only switches journaling mode.
 * @param db_file_path Path to the SQLite database file
 * @return Return status code
 */
Return db_migrate_from_2_to_3(const char *db_file_path)
{
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	char *err_msg = NULL;

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Database migration is not required\n");
		provide(status);
	}

	int rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Failed to open database: %s\n",sqlite3_errmsg(db));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const char *pragmas =
		        "PRAGMA locking_mode=EXCLUSIVE;"
		        "PRAGMA strict=ON;"
		        "PRAGMA fsync=ON;"
		        "PRAGMA synchronous=EXTRA;";

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
		sqlite3_stmt *stmt = NULL;

		rc = sqlite3_prepare_v2(db,"PRAGMA journal_mode=DELETE;",-1,&stmt,NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to prepare journal_mode switch: %s\n",sqlite3_errmsg(db));
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
				slog(ERROR,"journal_mode switch did not return a row (%i): %s\n",rc,sqlite3_errmsg(db));
				status = FAILURE;
			}
		}

		if(stmt != NULL)
		{
			sqlite3_finalize(stmt);
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_exec(db,"PRAGMA wal_checkpoint(TRUNCATE);",NULL,NULL,NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to checkpoint WAL: %s\n",sqlite3_errmsg(db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(strcmp(db_file_path,config->db_primary_file_path) == 0)
		{
			config->db_primary_file_modified = true;
		}
	}

	call(db_close(db,&config->db_primary_file_modified));

	provide(status);
}
