#include "precizer.h"

/**
 * @brief Validate stored DB path prefixes against the current traversal roots
 *
 * @details Skips validation in compare mode. When the primary database has just
 * been created, no stored-prefix comparison is needed. Otherwise the function
 * compares the normalized roots in `config->roots` with prefixes stored in the
 * database. Prefixes missing from the current root set are recorded for later
 * cleanup when the user explicitly allows it
 *
 * If mismatches are found and `--force` is not enabled, the function returns
 * `WARNING` so the caller can stop before replacing stored path metadata. When
 * `--force` is enabled, execution continues and the current roots may be
 * written to the database. For example, opening an existing database with a
 * different traversal root warns before any path metadata is replaced
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

	/* In compare mode this function has nothing useful to validate.
	   Compare mode works with two already opened databases, so checking whether
	   the primary database roots should be replaced would be unrelated here */
	if(config->compare == true)
	{
		provide(status);
	}

	/* A user interrupt should stop this validation before any extra database work
	   starts. Returning the current status lets the regular shutdown path finish
	   cleanly after Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	/* The main select statement finds stored path prefixes that are not part of
	   the current command-line roots. The insert statement records each mismatch
	   in a temporary helper table for later cleanup decisions */
	sqlite3_stmt *select_stmt = NULL;
	sqlite3_stmt *insert_stmt = NULL;
	int rc = 0;

	/* The paths are assumed to match until SQLite returns at least one stored
	   prefix that is missing from the current normalized roots */
	bool paths_are_equal = true;

	/* The SELECT text is assembled dynamically because the number of roots is not
	   fixed. Values are still bound as SQL parameters, so path text is not pasted
	   directly into the SQL request */
	m_create(char,select_sql,MEMORY_STRING);

	/* Build the query that finds stored prefixes outside the current root set.
	   Each root becomes one placeholder in the NOT IN list */
	if(config->roots.length != 0)
	{
		status = m_concat_literal(select_sql,"SELECT ID FROM paths WHERE prefix NOT IN (");

		if(SUCCESS != status)
		{
			m_del(select_sql);
			provide(status);
		}

		bool has_previous_root = false;

		/* Add one SQL placeholder for each normalized traversal root. Commas are
		   inserted only between placeholders, which keeps the generated SQL valid
		   for any number of roots */
		m_string_array_foreach(conf(roots),root)
		{
			if(has_previous_root == true)
			{
				/* Add a separator between root parameters. This is only needed
				   after the first placeholder has already been written */
				status = m_concat_literal(select_sql,",");

				if(SUCCESS != status)
				{
					m_del(select_sql);
					provide(status);
				}
			}

			status = m_concat_literal(select_sql,"?");

			if(SUCCESS != status)
			{
				m_del(select_sql);
				provide(status);
			}

			has_previous_root = true;
		}

		/* Close the SQL request after all placeholders have been added. From this
		   point the statement text is ready for sqlite3_prepare_v2() */
		status = m_concat_literal(select_sql,");");

		if(SUCCESS != status)
		{
			m_del(select_sql);
			provide(status);
		}
	}

	/* Prepare the SELECT statement that will ask SQLite for saved prefixes which
	   are no longer present in the current root list */
	rc = sqlite3_prepare_v2(config->db,m_text(select_sql),-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement %s",m_text(select_sql));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Bind every current root to its matching placeholder. Binding keeps paths
		   with quotes, spaces, or other special characters safe and unambiguous */
		int bind_index = 1;

		m_string_array_foreach(conf(roots),root)
		{
			const char *root_path = m_text(root);
			size_t root_path_length = 0;

			/* Ask libmem for the exact string length before binding. SQLite then
			   receives the path and its byte count explicitly */
			if(SUCCESS == status)
			{
				status = m_string_length(root,&root_path_length);
			}

			/* Attach the root value to the prepared SELECT statement. A binding
			   failure is a technical error because validation cannot continue with
			   an incomplete root list */
			if(SUCCESS == status)
			{
				rc = sqlite3_bind_text(select_stmt,bind_index,root_path,(int)root_path_length,NULL);

				if(SQLITE_OK != rc)
				{
					log_sqlite_error(config->db,rc,NULL,"Error binding root path in select");
					status = FAILURE;
				}
			}

			/* Move to the next SQL placeholder only after the current root was
			   bound successfully */
			if(SUCCESS == status)
			{
				bind_index++;
			}

			/* Once a binding-related error happens, the remaining roots are no
			   longer useful for this statement. Stop the loop and let cleanup run */
			if(SUCCESS != status)
			{
				break;
			}
		}
	}

	if(SUCCESS == status)
	{
		/* Execute the SELECT and handle every stored prefix that is missing from
		   the current root set. Any returned row means the database and command
		   line describe different traversal roots */
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			sqlite3_int64 path_ID = -1;

			path_ID = sqlite3_column_int64(select_stmt,0);

			/* A valid path ID marks a real mismatch. Store it in the helper table
			   so later code can decide which database records are outside the new
			   root scope */
			if(path_ID != -1)
			{
				paths_are_equal = false;

				const char *insert_sql = "INSERT INTO the_path_id_does_not_exists (path_id) VALUES (?1);";

				/* Prepare a small insert statement for the mismatched path ID.
				   Keeping this step separate makes SQLite errors visible with the
				   exact operation that failed */
				rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

				if(SQLITE_OK != rc)
				{
					log_sqlite_error(config->db,rc,NULL,"Can't prepare insert statement");
					status = FAILURE;
				}

				/* Bind the path ID returned by the SELECT. The helper table stores
				   identifiers, not path strings, so later cleanup can work by DB
				   row identity */
				if(SUCCESS == status)
				{
					rc = sqlite3_bind_int64(insert_stmt,1,path_ID);

					if(SQLITE_OK != rc)
					{
						log_sqlite_error(config->db,rc,NULL,"Error binding value in insert");
						status = FAILURE;
					}
				}

				/* Write the mismatched path ID into the helper table. If this does
				   not finish with SQLITE_DONE, the database state is not reliable
				   enough to continue normal validation */
				if(SUCCESS == status)
				{
					rc = sqlite3_step(insert_stmt);

					if(rc != SQLITE_DONE)
					{
						log_sqlite_error(config->db,rc,NULL,"Insert statement didn't return DONE");
						status = FAILURE;
					}
				}

				/* Finalize the insert statement for this row before continuing.
				   This releases SQLite resources even when the row caused an
				   error */
				sqlite3_finalize(insert_stmt);
			}
		}

		/* A SELECT loop must end with SQLITE_DONE. Any other result means SQLite
		   stopped for an error, so the validation result cannot be trusted */
		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	/* Release the dynamically assembled SQL text before leaving the database
	   scan section. The prepared statement owns its compiled copy already */
	m_del(select_sql);

	/* Finalize the SELECT statement after all rows were processed. This is safe
	   even when preparation failed because the statement pointer starts as NULL */
	sqlite3_finalize(select_stmt);

	if(SUCCESS == status)
	{
		/* A brand new primary database has no old path prefixes to protect.
		   There is no meaningful mismatch to report because the current roots are
		   the first roots this database has ever seen */
		if(config->db_primary_file_exists == false && config->compare == false)
		{
			slog(TRACE,"The brand new database has just been created. No need to verify the paths stored in the database against those passed as command-line arguments\n");
			provide(status);

		} else {

			if(paths_are_equal == true)
			{
				/* No stored prefix falls outside the current root list. The user
				   can continue because the command-line paths match the database
				   scope */
				slog(TRACE,"The paths written against the database and the paths passed as arguments are completely identical\n");
			} else {
				/* At least one saved prefix is outside the current roots. Warn the
				   user because continuing may replace path metadata and make old
				   file records unreachable */
				slog(EVERY,"The paths passed as arguments differ from those saved in the database. File paths and checksum information may be lost!\n");

				if(!(rational_logger_mode & SILENT))
				{
					/* In normal output modes, show the user exactly which prefixes
					   are already stored in the database. Silent mode skips this
					   explanatory list */
					slog(EVERY,"Paths saved in the database:\n");

					sqlite3_stmt *stmt = NULL;
					int rc_stmt = 0;
					char const *sql = "SELECT prefix FROM paths;";

					/* Prepare a simple display query for the stored prefixes. This
					   query is only for the warning message shown to the user */
					rc_stmt = sqlite3_prepare_v2(config->db,sql,-1,&stmt,NULL);

					if(SQLITE_OK != rc_stmt)
					{
						log_sqlite_error(config->db,rc_stmt,NULL,"Can't prepare select statement %s",sql);
						status = FAILURE;
					}

					if(SUCCESS == status)
					{
						/* Print each stored prefix without logger decorations so
						   the list is easy to read or copy */
						while(SQLITE_ROW == (rc_stmt = sqlite3_step(stmt)))
						{
							const char *prefix = (const char *)sqlite3_column_text(stmt,0);

							slog(EVERY|UNDECOR,"%s\n",prefix);
						}

						/* The display query must also end cleanly. If it does not,
						   report the SQLite error instead of presenting an
						   incomplete list as trustworthy */
						if(SQLITE_DONE != rc_stmt)
						{
							log_sqlite_error(config->db,rc_stmt,NULL,"Select statement didn't finish with DONE");
							status = FAILURE;
						}
					}

					/* Release the display query statement after the saved-prefix
					   list has been printed or skipped because of an error */
					sqlite3_finalize(stmt);
				}

				if(config->force == true)
				{
					/* With --force the user explicitly accepts replacing the stored
					   path scope. Show the new roots that will be written unless
					   output is intentionally silent */
					if(!(rational_logger_mode & SILENT))
					{
						slog(EVERY,"The " BOLD "--force" RESET " option has been used, so the following paths will be written to the %s:\n",confstr(db_file_name));

						/* Show the normalized traversal roots that will replace
						   stored prefixes. These are the paths the program will
						   treat as the database scope from now on */
						m_string_array_foreach(conf(roots),root)
						{
							slog(EVERY|UNDECOR,"%s\n",m_text(root));
						}
					}
				} else {
					/* Without --force the safe choice is to stop with a warning.
					   This gives the user a chance to confirm the path replacement
					   instead of losing metadata by accident */
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
