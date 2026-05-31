#include "precizer.h"

/**
 * @brief Validate stored DB path prefixes against the current traversal roots
 *
 * @details Skips validation in compare mode. When the primary database has just
 * been created, no stored-prefix comparison is needed. Otherwise the function
 * compares the normalized root descriptors in `config->roots` with prefixes
 * stored in the database `paths` table. Prefix IDs missing from the current
 * roots are recorded in `the_path_id_does_not_exists` so cleanup code can act
 * on them later when the user explicitly allows it
 *
 * If mismatches are found and `--force` is not enabled, the function returns
 * `WARNING` so the caller can stop before replacing stored path metadata. When
 * `--force` is enabled, execution continues and the current roots may be
 * written to the database. For example, opening an existing database created
 * for `/old/tree` with the root `/new/tree` warns before any path metadata is
 * replaced
 *
 * @warning Using `--force` incorrectly can replace path, file, and checksum
 * metadata in the database
 *
 * @return `SUCCESS` when prefixes match or forced path replacement is allowed,
 *         `WARNING` when mismatches are found without `--force`, or `FAILURE`
 *         on SQLite or memory errors
 */
Return db_validate_paths(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		provide(status);
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

	m_create(char,select_sql,MEMORY_STRING);

	// Create the SQL request from the current libmem-managed traversal roots
	if(config->roots.length != 0)
	{
		status = m_concat_literal(select_sql,"SELECT ID FROM paths WHERE prefix NOT IN (");

		if(SUCCESS != status)
		{
			m_del(select_sql);
			provide(status);
		}

		bool has_previous_root = false;

		m_string_array_foreach(conf(roots),root)
		{
			if(has_previous_root == true)
			{
				// Separate quoted root prefixes inside the SQL IN-list
				status = m_concat_literal(select_sql,",");

				if(SUCCESS != status)
				{
					m_del(select_sql);
					provide(status);
				}
			}

			m_create(char,path,MEMORY_STRING);

			run(db_sql_wrap_string(path,m_text(root)));

			run(m_concat_strings(select_sql,path));

			m_del(path);

			if(SUCCESS != status)
			{
				m_del(select_sql);
				provide(status);
			}

			has_previous_root = true;
		}

		// Close the SQL request string
		status = m_concat_literal(select_sql,");");

		if(SUCCESS != status)
		{
			m_del(select_sql);
			provide(status);
		}
	}

	rc = sqlite3_prepare_v2(config->db,m_text(select_sql),-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement %s",m_text(select_sql));
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

				const char *insert_sql = "INSERT INTO the_path_id_does_not_exists (path_id) VALUES (?1);";

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
					rc = sqlite3_step(insert_stmt);

					if(rc != SQLITE_DONE)
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

	m_del(select_sql);

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

							slog(EVERY|UNDECOR,"%s\n",prefix);
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
						slog(EVERY,"The " BOLD "--force" RESET " option has been used, so the following paths will be written to the %s:\n",confstr(db_file_name));

						// Show the normalized traversal roots that will replace stored prefixes
						m_string_array_foreach(conf(roots),root)
						{
							slog(EVERY|UNDECOR,"%s\n",m_text(root));
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
