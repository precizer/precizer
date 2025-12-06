#include "precizer.h"

/**
 *
 * Remove information from the database about files that had been deleted
 * on the file system or have been ignored
 *
 */
Return db_delete_missing_metadata(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	/* Skip in comparison mode */
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. The primary database does not require cleanup\n");
		provide(status);
	}

	/* Update mode should be enabled */
	if(config->update == true)
	{
		slog(EVERY,"Searching for files that no longer exist on the file system…\n");

	} else {
		// Don't do anything
		provide(status);
	}

	if(config->dry_run == true && config->db_primary_file_exists == true)
	{
		slog(TRACE,"Dry Run mode is enabled. The primary database must not be modified\n");
	}

	bool first_iteration = true;

	sqlite3_stmt *select_stmt = NULL;

	int rc = 0;

#if 0 // Old multiPATH solutions
	const char *select_sql = "SELECT files.ID,paths.prefix,files.relative_path FROM files LEFT JOIN paths ON files.path_prefix_index = paths.ID;";
#endif
	const char *select_sql = "SELECT files.ID,paths.prefix,files.relative_path FROM files,paths;";

	rc = sqlite3_prepare_v2(config->db,select_sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement");
		status = FAILURE;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			break;
		}

		sqlite_int64 ID = sqlite3_column_int64(select_stmt,0);
		const char *runtime_path_prefix = NULL;
		runtime_path_prefix = (const char *)sqlite3_column_text(select_stmt,1);
		const char *relative_path = NULL;
		relative_path = (const char *)sqlite3_column_text(select_stmt,2);
		char *absolute_path = NULL;

		bool path_was_removed_from_db = false;
		bool clean_ignored = false;

		if(runtime_path_prefix != NULL && relative_path != NULL)
		{
			/*
			 * Remove from the database mention of
			 * files that matches the regular expression
			 * passed through the ignore option(s)
			 *
			 */
			if(config->db_clean_ignored == true)
			{
				/*
				 *
				 * PCRE2 regexp to include the file
				 *
				 */

				// Don't show extra messages
				bool showed_once = true;

				Include response = include(relative_path,&showed_once);

				if(DO_NOT_INCLUDE == response)
				{
					/*
					 *
					 * PCRE2 regexp to ignore the file
					 *
					 */

					Ignore result = ignore(relative_path,&showed_once);

					if(IGNORE == result)
					{
						clean_ignored = true;

					} else if(FAIL_REGEXP_IGNORE == result){
						status = FAILURE;
					}

				} else if(FAIL_REGEXP_INCLUDE == response){
					status = FAILURE;
					break;
				}
			}

			if(strlen(runtime_path_prefix) > 0 && strlen(relative_path) > 0)
			{
				int length = asprintf(&absolute_path,"%s/%s",runtime_path_prefix,relative_path);

				if(length == -1)
				{
					slog(ERROR,"Generating the absolute path failed\n");
					status = FAILURE;
					break;
				}
			} else {
				slog(ERROR,"A zero-length path has been found\n");
				status = FAILURE;
				break;
			}
		} else {
			path_was_removed_from_db = true;
		}

		if(clean_ignored == true || path_was_removed_from_db == true || (absolute_path != NULL && access(absolute_path,F_OK) != 0))
		{
			status = db_delete_the_record_by_id(&ID,&first_iteration,&clean_ignored,relative_path);
		}

		free(absolute_path);
	}

	if(SQLITE_DONE != rc)
	{
		if(global_interrupt_flag == false)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	sqlite3_finalize(select_stmt);

	slog(EVERY,"Missing file search completed\n");

	provide(status);
}
