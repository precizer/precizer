  #include "precizer.h"

/**
 *
 * Save the directory prefix path into DB
 *
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

		const char *delete_sql="DELETE FROM paths WHERE ID IN (SELECT path_id FROM runtime_paths_id.the_path_id_does_not_exists);";

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
		const char *insert_sql    = "INSERT OR IGNORE INTO paths(prefix) VALUES(?1);";
		sqlite3_stmt *insert_stmt = NULL;

		for(int i = 0; config->paths[i]; i++)
		{

			// Remove unnecessary trailing slash at the end of the directory prefix
			remove_trailing_slash(config->paths[i]);

			/* Create SQL statement. Prepare to write */
			int rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

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
		}

		sqlite3_finalize(insert_stmt);

	}

	return(status);
}
