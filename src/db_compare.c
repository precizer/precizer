#include "precizer.h"

/**
 * @brief Attaches a secondary database to the primary database connection.
 *
 * The path comes from the two `--compare` positional arguments. SQLite attaches
 * the selected database under an internal schema alias such as `db1` or `db2`
 *
 * @param[in] db_A Index of the database path in `config->db_file_paths`.
 * @param[in] db_B Database number (1 or 2) to be used in the ATTACH DATABASE command.
 * @return Return structure indicating the operation status.
 */
static Return db_attach(
	int db_A,
	int db_B)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,sql,MEMORY_STRING);
	sqlite3_stmt *stmt = NULL;

	const memory *db_file_path = m_item_ro(memory,conf(db_file_paths),db_A);
	size_t db_file_path_length = 0;

	if(db_file_path == NULL)
	{
		report("Database path item is unavailable for index: %d",db_A);
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = m_string_length(db_file_path,&db_file_path_length);
	}

	if(SUCCESS == status)
	{
		run(m_formatted_string(sql,"ATTACH DATABASE ?1 AS db%d;",db_B));
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_prepare_v2(config->db,m_text(sql),-1,&stmt,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to prepare attach statement");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_bind_text(stmt,1,m_text(db_file_path),(int)db_file_path_length,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind database path in attach");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Attach statement didn't return DONE");
			status = FAILURE;
		}
	}

	int rc = sqlite3_finalize(stmt);

	if(SUCCESS == status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize attach statement");
		status = FAILURE;
	}

	call(m_del(sql));

	provide(status);
}

/**
 * @brief Detach database by alias
 *
 * @param[in] db_alias Attached database alias name
 * @return Return status code
 */
static Return db_detach(const char *db_alias)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3_stmt *stmt = NULL;

	const char *sql = "DETACH DATABASE ?1;";

	int rc = sqlite3_prepare_v2(config->db,sql,-1,&stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to prepare detach statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_text(stmt,1,db_alias,-1,SQLITE_STATIC);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind database alias in detach");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_step(stmt);

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Detach statement didn't return DONE");
			status = FAILURE;
		}
	}

	rc = sqlite3_finalize(stmt);

	if(SUCCESS == status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize detach statement");
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Run one compare query and print only the paths that pass output filters
 *
 * Executes the supplied comparison query, applies the shared --include/--ignore
 * decision before reporting each row, and reports only visible relative paths.
 * Only visible paths contribute to category state, so compare summaries and
 * equality messages are evaluated against the filtered scope. The category
 * heading is emitted only before the first visible path and stays visible in
 * `--silent` only when `show_headings_in_silent` enables it
 *
 * @param[in] compare_sql SQL query that returns relative paths for one compare category
 * @param[out] differences_found Set to `true` after the first visible path is printed
 *             so hidden rows stay outside the reported comparison scope
 * @param[in] show_headings_in_silent True to keep the category heading visible in `--silent`
 * @param[in] heading_format Heading format string with two `%s` database-name slots
 * @param[in] db_A_name First database name for the heading
 * @param[in] db_B_name Second database name for the heading
 * @return Return structure indicating the operation status
 */
static Return db_report_category(
	const char *compare_sql,
	bool       *differences_found,
	const bool show_headings_in_silent,
	const char *heading_format,
	const char *db_A_name,
	const char *db_B_name)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3_stmt *select_stmt = NULL;

	int rc = sqlite3_prepare_v2(config->db,compare_sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		bool first_visible_path = true;
		m_create(char,relative_path,MEMORY_STRING);

		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			// Interrupt the loop smoothly
			// Interrupt when Ctrl+C
			if(global_interrupt_flag == true)
			{
				break;
			}

			const unsigned char *db_relative_path = sqlite3_column_text(select_stmt,0);

			if(db_relative_path == NULL)
			{
				rc = sqlite3_errcode(config->db);
				log_sqlite_error(config->db,rc,NULL,"Failed to read relative path from select result");
				status = FAILURE;
				break;
			}

			size_t relative_path_size = (size_t)sqlite3_column_bytes(select_stmt,0) + 1U;

			run(m_copy_fixed_string(relative_path,relative_path_size,db_relative_path));

			if(SUCCESS != status)
			{
				break;
			}

			bool ignore = false;

			status = match_include_ignore(relative_path,
				NULL,
				&ignore);

			if(SUCCESS != status)
			{
				break;
			}

			if(ignore == true)
			{
				continue;
			}

			if(first_visible_path == true)
			{
				first_visible_path = false;

				// Outside --silent the heading is always shown
				// In --silent it stays only when multiple compare categories can mix together in one output
				if((rational_logger_mode & SILENT) == 0 || show_headings_in_silent == true)
				{
					slog(EVERY|VISIBLE_IN_SILENT,heading_format,db_A_name,db_B_name);
				}
			}

			*differences_found = true;
			slog(EVERY|UNDECOR|VISIBLE_IN_SILENT,"%s\n",m_text(relative_path));
		}

		m_del(relative_path);

		if(SUCCESS == status && global_interrupt_flag == false && SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	rc = sqlite3_finalize(select_stmt);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize SQLite statement");
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Compare the two databases selected by `--compare`
 *
 * @details The two database file paths come from `config->db_file_paths`, not
 * from traversal roots. The function verifies both files, attaches them to the
 * primary SQLite connection, reports requested difference categories, and
 * prints summary lines for missing paths and checksum mismatches
 *
 * The comparison scope can be limited with `--compare-filter` and further
 * narrowed by `--ignore` and `--include`; without filters the function checks
 * first-source paths, second-source paths, and SHA512 mismatches. For example,
 * `precizer --compare first.db second.db --compare-filter=sha512` validates both DB
 * files and reports checksum differences for paths present in both databases
 *
 * @return Return status code
 */
Return db_compare(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	bool attached_db1 = false;
	bool attached_db2 = false;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	/* Skip if comparison mode is not enabled */
	if(config->compare != true)
	{
		slog(TRACE,"Database comparison mode is not enabled. Skipping comparison\n");
		provide(status);
	}

	slog(EVERY,"The comparison of %s and %s databases is starting…\n",config->db_file_names[0],config->db_file_names[1]);

	/* Validate database paths */
	m_string_array_foreach(conf(db_file_paths),db_file_path)
	{
		const char *db_file_path_text = m_text(db_file_path);

		if(NOT_FOUND == file_availability(db_file_path_text,NULL,SHOULD_BE_A_FILE))
		{
			slog(ERROR,"The database file %s is either inaccessible or not a valid file\n",
				db_file_path_text);
			status = FAILURE;
			break;
		}

		if(SUCCESS == status)
		{
			/*
			 * Validate the integrity of the database file
			 */
			status = db_integrity_check(db_file_path_text);

			if(SUCCESS != status)
			{
				break;
			}
		}
	}

	/* Attach databases */
	if(SUCCESS == status)
	{
		// Attach the database 1
		status = db_attach(0,1);

		if(SUCCESS == status)
		{
			attached_db1 = true;
		}
	}

	if(SUCCESS == status)
	{
		// Attach the database 2
		status = db_attach(1,2);

		if(SUCCESS == status)
		{
			attached_db2 = true;
		}
	}

	/* SQL queries for comparison */
	const char *compare_A_sql = "SELECT a.relative_path "
	        "FROM db2.files AS a "
	        "LEFT JOIN db1.files AS b on b.relative_path = a.relative_path "
	        "WHERE b.relative_path IS NULL "
	        "ORDER BY a.relative_path ASC;";

	const char *compare_B_sql = "SELECT a.relative_path "
	        "FROM db1.files AS a "
	        "LEFT join db2.files AS b on b.relative_path = a.relative_path "
	        "WHERE b.relative_path IS NULL "
	        "ORDER BY a.relative_path ASC;";

	// True when user provided at least one --compare-filter option.
	// False means default compare mode: all three categories are enabled.
	const bool filter_specified = config->compare_filter != CF_NONE_SPECIFIED;

	// Enables "first-source" category:
	// show paths that exist in db1 but are missing in db2.
	// This category is active either explicitly by filter or by default mode.
	const bool check_first_source = (config->compare_filter & CF_FIRST_SOURCE)
	        || filter_specified == false;

	// Enables "second-source" category:
	// show paths that exist in db2 but are missing in db1.
	// This category is active either explicitly by filter or by default mode.
	const bool check_second_source = (config->compare_filter & CF_SECOND_SOURCE)
	        || filter_specified == false;

	// Enables checksum verification category for common relative paths.
	// Active either explicitly by checksum filter or by default mode.
	const bool verify_checksum_consistency = (config->compare_filter & CF_CHECKSUM_MISMATCH)
	        || filter_specified == false;

	// Counts enabled compare categories so silent mode can decide whether headings are needed
	unsigned int active_compare_categories = 0u;

	if(check_first_source == true)
	{
		active_compare_categories++;
	}

	if(check_second_source == true)
	{
		active_compare_categories++;
	}

	if(verify_checksum_consistency == true)
	{
		active_compare_categories++;
	}

	// Keeps category headings visible only when silent output can mix multiple categories
	const bool show_headings_in_silent = active_compare_categories > 1u;

	// Comparison result flags grouped in one place for summary evaluation
	bool first_source_differences_found = false;
	bool second_source_differences_found = false;
	bool checksum_mismatches_found = false;

	/* Compare files existence between databases */
	if(check_first_source == true)
	{
		run(db_report_category(compare_B_sql,
			&first_source_differences_found,
			show_headings_in_silent,
			BOLD "These files are no longer in the %s but still exist in the %s" RESET "\n",
			config->db_file_names[1],
			config->db_file_names[0]));
	}

	if(check_second_source == true)
	{
		run(db_report_category(compare_A_sql,
			&second_source_differences_found,
			show_headings_in_silent,
			BOLD "These files are no longer in the %s but still exist in the %s" RESET "\n",
			config->db_file_names[0],
			config->db_file_names[1]));
	}

#if 0
	// Disabled multi-root path index implementation
	const char *compare_checksums_sql = "select a.relative_path from db2.files a inner join db1.files b"
	        " on b.relative_path = a.relative_path "
	        " and b.sha512 is not a.sha512"
	        " order by a.relative_path asc;";

	const char *compare_checksums_sql = "SELECT p.path,f1.relative_path "
	        "FROM db1.files AS f1 "
	        "JOIN db1.paths AS p ON f1.path_prefix_index = p.ID "
	        "JOIN db2.files AS f2 ON f1.relative_path = f2.relative_path "
	        "JOIN db2.paths AS p2 ON f2.path_prefix_index = p2.ID "
	        "WHERE f1.sha512 IS NOT f2.sha512 AND p.path = p2.path "
	        "ORDER BY p.path,f1.relative_path ASC;";
#else
	// One PATH solution
	const char *compare_checksums_sql = "SELECT a.relative_path "
	        "FROM db2.files AS a "
	        "INNER JOIN db1.files AS b ON b.relative_path = a.relative_path "
	        "WHERE b.sha512 IS NOT a.sha512 "
	        "ORDER BY a.relative_path ASC;";
#endif

	if(verify_checksum_consistency == true)
	{
		run(db_report_category(compare_checksums_sql,
			&checksum_mismatches_found,
			show_headings_in_silent,
			BOLD "The SHA512 checksums of these files do not match between %s and %s" RESET "\n",
			config->db_file_names[0],
			config->db_file_names[1]));
	}

	/* Cleanup */
	if(attached_db1 == true)
	{
		call(db_detach("db1"));
	}

	if(attached_db2 == true)
	{
		call(db_detach("db2"));
	}

	/* Output results */
	if(SUCCESS == status)
	{
		const bool full_compare_scope = check_first_source == true
		        && check_second_source == true
		        && verify_checksum_consistency == true;

		if(full_compare_scope == true
		        && first_source_differences_found == false
		        && second_source_differences_found == false
		        && checksum_mismatches_found == false)
		{
			slog(EVERY,BOLD "All files are identical against %s and %s" RESET "\n",
				config->db_file_names[0],
				config->db_file_names[1]);

		} else if(full_compare_scope == false){

			if(check_first_source == true
			        && first_source_differences_found == false)
			{
				slog(EVERY,BOLD "No first-source differences found between %s and %s" RESET "\n",
					config->db_file_names[0],
					config->db_file_names[1]);
			}

			if(check_second_source == true
			        && second_source_differences_found == false)
			{
				slog(EVERY,BOLD "No second-source differences found between %s and %s" RESET "\n",
					config->db_file_names[0],
					config->db_file_names[1]);
			}
		}

		if(verify_checksum_consistency == true && checksum_mismatches_found == false)
		{
			slog(EVERY,BOLD "All SHA512 checksums of files are identical against %s and %s" RESET "\n",
				config->db_file_names[0],
				config->db_file_names[1]);
		}

		if(full_compare_scope == true
		        && first_source_differences_found == false
		        && second_source_differences_found == false
		        && checksum_mismatches_found == false)
		{
			slog(EVERY,BOLD "The databases %s and %s are absolutely equal" RESET "\n",
				config->db_file_names[0],
				config->db_file_names[1]);
		}
	}

	slog(EVERY,"Comparison of %s and %s databases is complete\n",
		config->db_file_names[0],
		config->db_file_names[1]);

	provide(status);
}
