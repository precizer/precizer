#include "precizer.h"
#include <unistd.h>

/**
 *
 * Remove information from the database about files that had been deleted
 * on the file system
 *
 */
Return db_delete_missing_files_from(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	int rc = 0;

#if 0 // Old multiPATH solutions
	const char *select_sql = "SELECT files.ID,paths.prefix,files.relative_path FROM files LEFT JOIN paths ON files.path_prefix_index = paths.ID;";
#endif
	const char *select_sql = "SELECT files.ID,paths.prefix,files.relative_path FROM files,paths;";

	rc = sqlite3_prepare_v2(config->db, select_sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	bool first_iteration = true;

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true){
			break;
		}

		sqlite_int64 ID = sqlite3_column_int64(select_stmt,0);
		const char *runtime_path_prefix = NULL;
		runtime_path_prefix = (const char *)sqlite3_column_text(select_stmt,1);
		const char *relative_path = NULL;
		relative_path = (const char *)sqlite3_column_text(select_stmt,2);
		char *absolute_path = NULL;

		bool path_was_removed_from_db = false;
		bool clear_ignored = false;

		if(runtime_path_prefix != NULL && relative_path != NULL){

			/*
			 * Remove from the database mention of
			 * files that matches the regular expression
			 * passed through the ignore option(s)
			 *
			 */
			if(config->db_clean_ignored)
			{
				/*
				 *
				 * PCRE2 regexp to ignore the file
				 *
				 */

				bool showed_once = true;

				// Default value
				Ignore result = ignore(relative_path,&showed_once);

				if(IGNORE == result)
				{
					clear_ignored = true;

				} else if (REGEXP_FAIL == result)
				{
					status = FAILURE;
					break;
				}
			}

			// The variable in the stack is extremely fast
			size_t runtime_path_prefix_size = strlen(runtime_path_prefix);
			size_t relative_path_size = strlen(relative_path);
			absolute_path = (char *)calloc(runtime_path_prefix_size + relative_path_size + 2,sizeof(char)); // One for '/' and second for '\0' at the end of the line
			if(absolute_path == NULL)
			{
				slog(false,"ERROR: Memory allocation did not complete successfully!\n");
				status = FAILURE;
				break;
			}
			strcpy(absolute_path,runtime_path_prefix);
			absolute_path[runtime_path_prefix_size] = '/';
			absolute_path[runtime_path_prefix_size + 1] = '\0';
			strcat(absolute_path,relative_path);
			absolute_path[runtime_path_prefix_size + relative_path_size + 1] = '\0';
		} else {
			path_was_removed_from_db = true;
		}

		if(clear_ignored == true || path_was_removed_from_db == true || access(absolute_path,F_OK) != 0)
		{
			status = db_delete_the_file_by_id(&ID,&first_iteration,&clear_ignored,relative_path);
		}
		free(absolute_path);
	}
	if(SQLITE_DONE != rc) {
		if(global_interrupt_flag == false)
		{
			slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	sqlite3_finalize(select_stmt);

	return(status);
}
