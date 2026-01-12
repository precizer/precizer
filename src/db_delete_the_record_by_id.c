#include "precizer.h"

/**
 *
 * This function remove information about a specific
 * file from the database by its unique db ID
 *
 */
Return db_delete_the_record_by_id(
	sqlite_int64 *ID,
	bool         *first_iteration,
	const bool   *clean_ignored,
	const char   *relative_path,
	const char   *runtime_path_prefix)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Indicates removal due to missing or unreadable path
	bool inaccessible = false;

	bool file_not_found = false;

	char *absolute_path = NULL;

	if(clean_ignored != NULL && *clean_ignored == false
	        && runtime_path_prefix != NULL && relative_path != NULL)
	{
		int length = asprintf(&absolute_path,"%s/%s",runtime_path_prefix,relative_path);

		if(length == -1)
		{
			free(absolute_path);
			return(FAILURE);
		}

		FileAccessStatus access_status = file_check_access(absolute_path,(size_t)length);

		free(absolute_path);

		if(access_status == FILE_ACCESS_ERROR)
		{
			return(FAILURE);
		}

		if(access_status == FILE_NOT_FOUND)
		{
			file_not_found = true;

		} else if(access_status == FILE_ACCESS_DENIED){

			inaccessible = true;

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

	int sql_return = SQLITE_DONE;

	if(SUCCESS == status)
	{
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
					if(config->db_clean_ignored == false)
					{
						slog(EVERY,"If the information about ignored files should be removed from the database the " BOLD "--db-clean-ignored" RESET " option must be specified. This is special protection against accidental deletion of information from the database\n");
					} else {
						slog(TRACE,"The " BOLD "--db-clean-ignored" RESET " option has been used, so the information about ignored files will be removed against the database %s\n",config->db_file_name);
					}
				}

				if(config->the_update_warning_has_already_been_shown == false)
				{
					slog(EVERY,"The " BOLD "--update" RESET " option has been used, so the information about files will be deleted against the database %s\n",config->db_file_name);
				}

				/* Reflect changes in global */
				if(config->dry_run == false)
				{
					config->db_primary_file_modified = true;
				}

				slog(EVERY,BOLD "These files are no longer exist or ignored and will be deleted against the DB %s:" RESET "\n",config->db_file_name);
			}

			if(*clean_ignored == true)
			{
				slog(EVERY|UNDECOR,"clean ignored %s\n",relative_path);

			} else if(inaccessible == true){

				slog(EVERY|UNDECOR,"inaccessible %s\n",relative_path);

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
