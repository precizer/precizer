#include "precizer.h"

/**
 *
 * Drop a file record from the database by its unique ID.
 * Records are dropped only for ignored paths, missing files, or inaccessible
 * paths when --db-drop-inaccessible is enabled; accessible paths remain.
 * In --dry-run mode, the database is not modified.
 *
 */
Return db_delete_the_record_by_id(
	const sqlite_int64 *ID,
	bool               *first_iteration,
	const bool         *drop_ignored,
	const char         *relative_path,
	const char         *runtime_path_prefix)
{
	/// The status that will be passed to provide() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Indicates removal due to missing or unreadable path
	bool inaccessible = false;

	bool file_not_found = false;

	char *absolute_path = NULL;

	if(drop_ignored != NULL && *drop_ignored == false
	        && runtime_path_prefix != NULL && relative_path != NULL)
	{
		int length = asprintf(&absolute_path,"%s/%s",runtime_path_prefix,relative_path);

		if(length == -1)
		{
			free(absolute_path);
			return(FAILURE);
		}

		FileAccessStatus access_status = file_check_access(absolute_path,(size_t)length,R_OK);

		free(absolute_path);

		if(access_status == FILE_ACCESS_ERROR)
		{
			return(FAILURE);
		}

		if(access_status == FILE_NOT_FOUND)
		{
			file_not_found = true;

		} else if(access_status == FILE_ACCESS_DENIED){

			if(config->db_drop_inaccessible == true)
			{
				inaccessible = true;

			} else {

				slog(EVERY|UNDECOR,"kept inaccessible %s\n",relative_path);

				return(SUCCESS);

			}

		} else if(access_status == FILE_ACCESS_ALLOWED){

			/* The file remains available.
			   Keep file references in the database! */
			return(SUCCESS);
		}
	}

	sqlite3_stmt *delete_stmt = NULL;
	int rc = 0;

	const char *sql = "DELETE FROM files WHERE ID=?1;";

	// Don't do anything in case of --dry-run
	if(config->dry_run == true)
	{
		sql = "SELECT ID FROM files WHERE ID=?1;";
	}

	rc = sqlite3_prepare_v2(config->db,sql,-1,&delete_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare delete statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(delete_stmt,1,*ID);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in delete");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int sql_return = SQLITE_DONE;

		// Select instead Delete in Dry Run mode
		if(config->dry_run == true)
		{
			sql_return = SQLITE_ROW;
		}

		/* Execute SQL statement */
		if(sqlite3_step(delete_stmt) == sql_return)
		{
			if(*first_iteration == true)
			{
				*first_iteration = false;

				if(config->ignore != NULL)
				{
					if(config->db_drop_ignored == false)
					{
						slog(EVERY,"If the information about ignored files should be removed from the database the " BOLD "--db-drop-ignored" RESET " option must be specified. This is special protection against accidental deletion of information from the database\n");
					} else {
						slog(TRACE,"The " BOLD "--db-drop-ignored" RESET " option has been used, so the information about ignored files will be removed against the database %s\n",confstr(db_file_name));
					}
				}

				/* Reflect changes in global */
				if(config->dry_run == false)
				{
					config->db_primary_file_modified = true;
				}

				if(config->db_drop_inaccessible)
				{
					slog(EVERY,BOLD "Dropping DB records for missing, inaccessible, or ignored paths in %s:" RESET "\n",confstr(db_file_name));
				} else {
					slog(EVERY,BOLD "Dropping DB records for missing or ignored paths in %s:" RESET "\n",confstr(db_file_name));
				}
			}

			if(*drop_ignored == true)
			{
				slog(EVERY|UNDECOR|REMEMBER,"drop ignored %s\n",relative_path);

			} else if(inaccessible == true){

				slog(EVERY|UNDECOR|REMEMBER,"drop due to inaccessible %s\n",relative_path);

			} else if(file_not_found == true){

				slog(EVERY|UNDECOR,"no longer exists %s\n",relative_path);

			} else {

				slog(ERROR,"An unexpected error that should never occur for %s\n",relative_path);
			}

		} else {

			log_sqlite_error(config->db,rc,NULL,"Delete statement didn't return right code %d",sql_return);
			status = FAILURE;
		}
	}

	sqlite3_finalize(delete_stmt);

	provide(status);
}
