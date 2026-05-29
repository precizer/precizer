/**
 * @file db_save_prefixes.c
 * @brief Database operations for directory prefix paths
 */

#include "precizer.h"

/**
 * @brief Save the current traversal roots into the `paths` table
 *
 * The positional directories accepted by normal scanning mode are stored in
 * `config->roots` as libmem string descriptors. This function writes each
 * normalized root into the database once, so later file records can be resolved
 * relative to those root prefixes. In `--compare` mode the function returns
 * immediately because compare arguments are database files rather than
 * traversal roots
 *
 * With `--force`, obsolete path rows are removed before the current roots are
 * inserted. With `--dry-run` against an already existing physical database,
 * inserts are skipped so the on-disk database is not modified
 *
 * For example, after parsing `precizer --database tree.db /home/me/tree`,
 * `config->roots` contains `/home/me/tree`, and this function ensures that
 * prefix exists in `paths.prefix`
 *
 * @return `SUCCESS` when all required prefixes are present or intentionally
 *         skipped by mode. `FAILURE` when SQLite preparation, binding, or
 *         execution fails
 */
Return db_save_prefixes(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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
		return(status);
	}

	if(config->force == true && config->dry_run == false)
	{
		/* Delete previous records in the table  */
		sqlite3_stmt *delete_stmt = NULL;

		const char *delete_sql = "DELETE FROM paths WHERE ID IN (SELECT path_id FROM the_path_id_does_not_exists);";

		int rc = sqlite3_prepare_v2(config->db,delete_sql,-1,&delete_stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Can't prepare delete statement");
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			/* Execute SQL statement */
			if(sqlite3_step(delete_stmt) != SQLITE_DONE)
			{
				log_sqlite_error(config->db,rc,NULL,"Delete statement didn't return DONE");
				status = FAILURE;
			}
		}

		if(SUCCESS == status)
		{
			/* Reflect changes in global */
			config->db_primary_file_modified = true;
		}

		sqlite3_finalize(delete_stmt);
	}

	/*
	 * Write prefixes only when the selected mode may change the active database.
	 * Dry-run scans against an existing physical DB keep the on-disk file intact
	 */
	if(!(config->dry_run == true && config->db_primary_file_exists == true))
	{
		// Insert every normalized traversal root that is not already present
		m_string_array_foreach(conf(roots),root)
		{
			const char *root_path = m_text(root);

			const char *select_sql = "SELECT COUNT(*) FROM paths WHERE prefix = ?1;";
			sqlite3_stmt *select_stmt = NULL;

			/* First check if prefix exists */
			int rc = sqlite3_prepare_v2(config->db,select_sql,-1,&select_stmt,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement %s",select_sql);
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				rc = sqlite3_bind_text(select_stmt,1,root_path,(int)strlen(root_path),NULL);

				if(SQLITE_OK != rc)
				{
					log_sqlite_error(config->db,rc,NULL,"Error binding value in select");
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
					log_sqlite_error(config->db,rc,NULL,"Can't prepare insert statement %s",insert_sql);
					status = FAILURE;
				}

				if(SUCCESS == status)
				{
					rc = sqlite3_bind_text(insert_stmt,1,root_path,(int)strlen(root_path),NULL);

					if(SQLITE_OK != rc)
					{
						log_sqlite_error(config->db,rc,NULL,"Error binding value in insert");
						status = FAILURE;
					}
				}

				/* Execute SQL statement */
				if(SUCCESS == status)
				{
					if(sqlite3_step(insert_stmt) != SQLITE_DONE)
					{
						log_sqlite_error(config->db,rc,NULL,"Insert statement didn't return DONE");
						status = FAILURE;
					}
				}

				if(SUCCESS == status && config->dry_run == false)
				{
					/* Reflect changes in global */
					config->db_primary_file_modified = true;
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
