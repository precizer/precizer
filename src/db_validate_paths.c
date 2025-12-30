#include "precizer.h"

/**
 * @brief Validates path consistency between database records and provided config variables
 *
 * @details This function performs validation of paths by comparing paths stored
 * in the database against paths provided as config variables. The validation
 * process involves:
 *
 * 1. Checking if paths stored in database match paths passed as config variables
 * 2. Warning user about potential data loss if paths mismatch
 * 3. Handling forced path updates when --force flag is used
 *
 * The function supports several operational modes:
 * - Normal mode: Validates paths and prevents updates if mismatch detected
 * - Force mode: Allows path updates even when mismatches are detected
 * - Compare mode: Skips validation entirely
 *
 * Database operations:
 * - Queries existing paths from database
 * - Creates temporary tables for tracking mismatched paths
 * - Performs path prefix comparisons
 *
 * @note This function is crucial for preventing accidental data loss by ensuring
 * path consistency before performing database operations.
 *
 * @warning Using --force flag can lead to loss or replacement of file and
 * checksum information if used incorrectly.
 *
 * Error handling:
 * - Memory allocation failures
 * - SQLite preparation and execution errors
 * - Path mismatch detection
 *
 * @provide Return Status code indicating operation result:
 *         - SUCCESS (0): Paths are valid or force flag is used
 *         - FAILURE (1): Path mismatch detected without force flag
 *                       or database operation error occurred
 */
Return db_validate_paths(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	sqlite3_stmt *select_stmt = NULL;
	sqlite3_stmt *insert_stmt = NULL;
	int rc = 0;

	bool paths_are_equal = true;

	create(char,select_sql);

	// Create the SQL request
	if(config->paths[0] != NULL)
	{
		char const *sql_1 = "SELECT ID FROM paths WHERE prefix NOT IN (";

		status = concat_literal(select_sql,sql_1);

		if(SUCCESS != status)
		{
			del(select_sql);
			provide(status);
		}

		for(int i = 0; config->paths[i]; i++)
		{
			create(char,path);

			run(db_sql_wrap_string(path,config->paths[i]));

			run(concat_literal(select_sql,getcstring(path)));

			del(path);

			if(SUCCESS != status)
			{
				del(select_sql);
				provide(status);
			}

			// Not the last path in the array
			if(config->paths[i + 1] != 0)
			{
				/* Concatenate with the comma character "," */
				status = concat_literal(select_sql,",");

				if(SUCCESS != status)
				{
					del(select_sql);
					provide(status);
				}
			}
		}

		/* Close the string that contains SQL request */

		/* Concatenate with the ");" characters */
		status = concat_literal(select_sql,");");

		if(SUCCESS != status)
		{
			del(select_sql);
			provide(status);
		}
	}

	rc = sqlite3_prepare_v2(config->db,getstring(select_sql),-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement %s",getstring(select_sql));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			sqlite3_int64 path_ID = -1;

			path_ID = sqlite3_column_int64(select_stmt,0);

			if(path_ID != -1)
			{
				paths_are_equal = false;

				const char *insert_sql = "INSERT INTO runtime_paths_id.the_path_id_does_not_exists (path_id) VALUES (?1);";

				rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

				if(SQLITE_OK != rc)
				{
					log_sqlite_error(config->db,rc,NULL,"Can't prepare insert statement");
					status = FAILURE;
				}

				if(SUCCESS == status)
				{
					rc = sqlite3_bind_int64(insert_stmt,1,path_ID);

					if(SQLITE_OK != rc)
					{
						log_sqlite_error(config->db,rc,NULL,"Error binding value in insert");
						status = FAILURE;
					}
				}

				if(SUCCESS == status)
				{
					/* Execute SQL statement */
					if(sqlite3_step(insert_stmt) != SQLITE_DONE)
					{
						log_sqlite_error(config->db,rc,NULL,"Insert statement didn't return DONE");
						status = FAILURE;
					}
				}

				sqlite3_finalize(insert_stmt);
			}
		}

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	del(select_sql);

	sqlite3_finalize(select_stmt);

	if(SUCCESS == status)
	{
		/* If the primary database was just created */
		if(config->db_primary_file_exists == false && config->compare == false)
		{
			slog(TRACE,"The brand new database has just been created. No need to verify the paths stored in the database against those passed as command-line arguments\n");
			provide(status);

		} else {

			if(paths_are_equal == true)
			{
				slog(TRACE,"The paths written against the database and the paths passed as arguments are completely identical\n");
			} else {
				slog(EVERY,"The paths passed as arguments differ from those saved in the database. File paths and checksum information may be lost!\n");

				if(!(rational_logger_mode & SILENT))
				{
					slog(EVERY,"Paths saved in the database:\n");

					sqlite3_stmt *stmt;
					int rc_stmt = 0;
					char const *sql = "SELECT prefix FROM paths;";

					rc_stmt = sqlite3_prepare_v2(config->db,sql,-1,&stmt,NULL);

					if(SQLITE_OK != rc_stmt)
					{
						log_sqlite_error(config->db,rc_stmt,NULL,"Can't prepare select statement %s",sql);
						status = FAILURE;
					}

					if(SUCCESS == status)
					{
						while(SQLITE_ROW == (rc_stmt = sqlite3_step(stmt)))
						{
							const char *prefix = (const char *)sqlite3_column_text(stmt,0);

							printf("%s\n",prefix);
						}

						if(SQLITE_DONE != rc_stmt)
						{
							log_sqlite_error(config->db,rc_stmt,NULL,"Select statement didn't finish with DONE");
							status = FAILURE;
						}
					}

					sqlite3_finalize(stmt);
				}

				if(config->force == true)
				{
					if(!(rational_logger_mode & SILENT))
					{
						slog(EVERY,"The " BOLD "--force" RESET " option has been used, so the following paths will be written to the %s:\n",config->db_file_name);

						for(int i = 0; config->paths[i]; i++)
						{
							printf("%s\n",config->paths[i]);
						}
					}
				} else {
					slog(EVERY,"Use the " BOLD "--force" RESET " option only when the PATHS stored in the database need"
						" to be updated. Warning: If this option is used incorrectly, file and checksum information"
						" in the database may be lost or completely replaced with different values.\n");
					status = WARNING;
				}
			}
		}
	}

	provide(status);
}
