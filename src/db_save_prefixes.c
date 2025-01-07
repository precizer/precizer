/**
 * @file precizer.h
 * @brief Database operations for directory prefix paths
 */

#include "precizer.h"

/**
 * @brief Saves directory prefix paths into the database
 *
 * @details This function handles the storage of directory prefix paths in the database
 * according to several operational modes:
 *
 * Operational Modes:
 * - Compare Mode: Function returns immediately with SUCCESS
 * - Force Mode: Previous records are deleted before insertion
 * - Dry Run Mode: Data insertion occurs in memory only
 *
 * Data Insertion Rules:
 * - Data is inserted only if:
 *   - Database file is not physical, OR
 *   - Dry Run mode is not activated
 * - In Dry Run mode with physical database:
 *   - Writing occurs to in-memory database only
 *
 * Processing Steps:
 * 1. Checks operational mode
 * 2. Handles record deletion if in Force mode
 * 3. Processes path insertions according to insertion rules
 * 4. Removes trailing slashes from paths before insertion
 *
 * Error Handling:
 * - SQLite preparation errors
 * - SQLite binding errors
 * - SQLite execution errors
 * All errors are logged using slog() function
 *
 * @pre config structure must be properly initialized with:
 *      - compare flag
 *      - force flag
 *      - dry_run flag
 *      - db connection
 *      - paths array
 *
 * @return Return enum value:
 *      - SUCCESS (0): Paths saved successfully
 *      - FAILURE (1): Operation failed due to database errors
 */
Return db_save_prefixes(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Skip in comparison mode */
	if(config->compare == true)
	{
		return(status);
	}

	if(config->force == true && config->dry_run == false)
	{
		/* Delete previous records in the table  */
		sqlite3_stmt *delete_stmt = NULL;

		const char *delete_sql = "DELETE FROM paths WHERE ID IN (SELECT path_id FROM runtime_paths_id.the_path_id_does_not_exists);";

		int rc = sqlite3_prepare_v2(config->db,delete_sql,-1,&delete_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Can't prepare delete statement (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			/* Execute SQL statement */
			if(sqlite3_step(delete_stmt) != SQLITE_DONE)
			{
				slog(ERROR,"Delete statement didn't return DONE (%i): %s\n",rc,sqlite3_errmsg(config->db));
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{
			/* Reflect changes in global */
			config->something_has_been_changed = true;
		}

		sqlite3_finalize(delete_stmt);
	}

	/**
	 * @brief Data insertion handling rules
	 * @details Data insertion occurs only if the database file is not a physical file
	 *          and Dry Run mode is not activated. With Dry Run mode activated,
	 *          if a physical file is not opened, writing occurs to an in-memory database.
	 */
	if(!(config->dry_run == true && config->db_file_exists == true))
	{
		for(int i = 0; config->paths[i]; i++)
		{
			// Remove unnecessary trailing slash at the end of the directory prefix
			remove_trailing_slash(config->paths[i]);

			const char *select_sql = "SELECT COUNT(*) FROM paths WHERE prefix = ?1;";
			sqlite3_stmt *select_stmt = NULL;

			/* First check if prefix exists */
			int rc = sqlite3_prepare_v2(config->db,select_sql,-1,&select_stmt,NULL);

			if(SQLITE_OK != rc)
			{
				slog(ERROR,"Can't prepare select statement %s (%i): %s\n",select_sql,rc,sqlite3_errmsg(config->db));
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				rc = sqlite3_bind_text(select_stmt,1,config->paths[i],(int)strlen(config->paths[i]),NULL);

				if(SQLITE_OK != rc)
				{
					slog(ERROR,"Error binding value in select (%i): %s\n",rc,sqlite3_errmsg(config->db));
					status = FAILURE;
				}
			}

			int count = 0;

			if(SUCCESS == status)
			{
				if(sqlite3_step(select_stmt) == SQLITE_ROW)
				{
					count = sqlite3_column_int(select_stmt,0);
				}
			}

			sqlite3_finalize(select_stmt);

			/* Only proceed with insert if prefix doesn't exist */
			if(count == 0)
			{
				const char *insert_sql = "INSERT OR IGNORE INTO paths(prefix) VALUES(?1);";
				sqlite3_stmt *insert_stmt = NULL;

				/* Create SQL statement. Prepare to write */
				rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

				if(SQLITE_OK != rc)
				{
					slog(ERROR,"Can't prepare insert statement %s (%i): %s\n",insert_sql,rc,sqlite3_errmsg(config->db));
					status = FAILURE;
				}

				if(SUCCESS == status)
				{
					rc = sqlite3_bind_text(insert_stmt,1,config->paths[i],(int)strlen(config->paths[i]),NULL);

					if(SQLITE_OK != rc)
					{
						slog(ERROR,"Error binding value in insert (%i): %s\n",rc,sqlite3_errmsg(config->db));
						status = FAILURE;
					}
				}

				/* Execute SQL statement */
				if(SUCCESS == status)
				{
					if(sqlite3_step(insert_stmt) != SQLITE_DONE)
					{
						slog(ERROR,"Insert statement didn't return DONE (%i): %s\n",rc,sqlite3_errmsg(config->db));
						status = FAILURE;
					}
				}

				if(SUCCESS == status)
				{
					/* Reflect changes in global */
					config->something_has_been_changed = true;
				}

				sqlite3_finalize(insert_stmt);
			}

			if(SUCCESS != status)
			{
				break;
			}
		}
	}

	return(status);
}
