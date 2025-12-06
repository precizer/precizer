/**
 * @file db_migrate_from_1_to_2.c
 * @brief
 */

#include "precizer.h"

/**
 * @brief Migrates database from version 1 to version 2
 * @param db_file_path Path to the SQLite database file
 * @return Return status code
 */
Return db_migrate_from_1_to_2(const char *db_file_path)
{
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	char *err_msg = NULL;

	if(config->dry_run == true)
	{
		slog(TRACE,"Dry Run mode is enabled. Database migration is not required\n");
		provide(status);
	}

	/* Open database in safe mode */
	int rc = sqlite3_open_v2(db_file_path,&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,NULL);

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
		/**
		 *
		 * If the database being updated is the primary one, adjust the global
		 * flag indicating that the main database file has been
		 * modified (this will consequently update the file's ctime, mtime, and size)
		 */
		if(strcmp(db_file_path,config->db_primary_file_path) == 0)
		{
			/* Changes have been made to the database. Update
			   this in the global variable value. */
			config->db_primary_file_modified = true;
		}
	}

	/* Cleanup */
	call(db_close(db,&config->db_primary_file_modified));

	provide(status);
}
