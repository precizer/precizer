#include "precizer.h"

/**
 * @brief Composes an SQL ATTACH DATABASE query string.
 *
 * This function generates an SQL query string to attach a database with a specified path and number.
 *
 * @param[out] sql Pointer to a string that will hold the generated SQL query.
 * @param[in] db_path Path to the database file to be attached.
 * @param[in] db_num Database number (1 or 2) used in the query.
 * @return Return structure indicating the operation status.
 */
static Return compose_sql(
	char       **sql,
	const char *db_path,
	int        db_num)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	create(char,wrapped_db_path);

	run(db_sql_wrap_string(wrapped_db_path,db_path));

	if(SUCCESS == status && asprintf(sql,"ATTACH DATABASE %s as db%d;",getcstring(wrapped_db_path),db_num) == -1)
	{
		status = FAILURE;
		report("Memory allocation failed for SQL query string");
	}

	del(wrapped_db_path);

	provide(status);
}

/**
 * @brief Attaches a secondary database to the primary database connection.
 *
 * This function attaches a secondary database (specified by its index in the configuration)
 * to the primary SQLite database connection using the ATTACH DATABASE command.
 *
 * @param[in] db_A Index of the database path in the configuration array.
 * @param[in] db_B Database number (1 or 2) to be used in the ATTACH DATABASE command.
 * @return Return structure indicating the operation status.
 */
static Return db_attach(
	int db_A,
	int db_B)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	char *select_sql = NULL;

	run(compose_sql(&select_sql,config->db_file_paths[db_A],db_B));

	if(SUCCESS == status)
	{
		int rc = sqlite3_exec(config->db,select_sql,NULL,NULL,NULL);

		if(rc!= SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,"Can't execute");
			status = FAILURE;
		}
	}

	free(select_sql);

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

	if(stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}

	provide(status);
}

/**
 * @brief Compares changes between two databases.
 *
 * This function executes a provided SQL query to compare differences between two databases.
 * It identifies files that exist in one database but not the other, updating flags to reflect the comparison results.
 *
 * @param[in] compare_sql SQL query string for comparison.
 * @param[out] differences_found Flag indicating whether at least one difference was found for this query.
 * @param[in] db_A Index of the first database in the configuration array.
 * @param[in] db_B Index of the second database in the configuration array.
 * @return Return structure indicating the operation status.
 */
static Return db_changes(
	const char *compare_sql,
	bool       *differences_found,
	int        db_A,
	int        db_B)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	bool first_iteration = true;

	sqlite3_stmt *select_stmt = NULL;

	int rc = sqlite3_prepare_v2(config->db,compare_sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			*differences_found = true;

			// Interrupt the loop smoothly
			// Interrupt when Ctrl+C
			if(global_interrupt_flag == true)
			{
				break;
			}

			if(first_iteration == true)
			{
				first_iteration = false;
				slog(EVERY,BOLD "These files are no longer in the %s but still exist in the %s" RESET "\n",config->db_file_names[db_A],config->db_file_names[db_B]);
			}

			const unsigned char *relative_path = NULL;
			relative_path = sqlite3_column_text(select_stmt,0);

			if(relative_path != NULL)
			{
				slog(EVERY|UNDECOR,"%s\n",relative_path);
			} else {
				rc = sqlite3_errcode(config->db);
				log_sqlite_error(config->db,rc,NULL,"Failed to read relative path from select result");
				status = FAILURE;
				break;
			}
		}

		if(SUCCESS == status && global_interrupt_flag == false && SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}

	if(select_stmt != NULL)
	{
		rc = sqlite3_finalize(select_stmt);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to finalize SQLite statement");
			status = FAILURE;
		} else {
			select_stmt = NULL;
		}
	}

	provide(status);
}

/**
 * @brief Compare two databases
 * @details Compares content of two databases specified in Config structure
 *          Checks for file existence, missing files and SHA512 checksums
 * @return Return enum indicating operation status
 */
Return db_compare(void)
{
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

	slog(EVERY,"The comparison of %s and %s databases is starting…\n",
		config->db_file_names[0],
		config->db_file_names[1]);

	/* Validate database paths */
	for(int i = 0; config->db_file_paths[i]; i++)
	{
		if(NOT_FOUND == file_availability(config->db_file_paths[i],SHOULD_BE_A_FILE))
		{
			slog(ERROR,"The database file %s is either inaccessible or not a valid file\n",
				config->db_file_paths[i]);
			status = FAILURE;
			break;
		}

		if(SUCCESS == status)
		{
			/*
			 * Validate the integrity of the database file
			 */
			status = db_test(config->db_file_paths[i]);

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
	const bool filter_specified = config->compare_filter_checksum_mismatch == true
	        || config->compare_filter_second_source_only == true
	        || config->compare_filter_first_source_only == true;

	// Enables "first-source-only" category:
	// show paths that exist in db1 but are missing in db2.
	// This category is active either explicitly by filter or by default mode.
	const bool check_first_source_only = config->compare_filter_first_source_only == true
	        || filter_specified == false;

	// Enables "second-source-only" category:
	// show paths that exist in db2 but are missing in db1.
	// This category is active either explicitly by filter or by default mode.
	const bool check_second_source_only = config->compare_filter_second_source_only == true
	        || filter_specified == false;

	// Enables checksum verification category for common relative paths.
	// Active either explicitly by checksum filter or by default mode.
	const bool verify_checksum_consistency = config->compare_filter_checksum_mismatch == true
	        || filter_specified == false;

	// Comparison result flags grouped in one place for summary evaluation
	bool first_source_only_differences_found = false;
	bool second_source_only_differences_found = false;
	bool checksum_mismatches_found = false;

	/* Compare files existence between databases */
	if(check_first_source_only == true)
	{
		run(db_changes(compare_B_sql,
			&first_source_only_differences_found,
			1,
			0));
	}

	if(check_second_source_only == true)
	{
		run(db_changes(compare_A_sql,
			&second_source_only_differences_found,
			0,
			1));
	}

#if 0
	// Old multiPATH solutions
	const char *compare_checksums = "select a.relative_path from db2.files a inner join db1.files b"
	        " on b.relative_path = a.relative_path "
	        " and b.sha512 is not a.sha512"
	        " order by a.relative_path asc;";

	const char *compare_checksums = "SELECT p.path,f1.relative_path "
	        "FROM db1.files AS f1 "
	        "JOIN db1.paths AS p ON f1.path_prefix_index = p.ID "
	        "JOIN db2.files AS f2 ON f1.relative_path = f2.relative_path "
	        "JOIN db2.paths AS p2 ON f2.path_prefix_index = p2.ID "
	        "WHERE f1.sha512 IS NOT f2.sha512 AND p.path = p2.path "
	        "ORDER BY p.path,f1.relative_path ASC;";
#else
	// One PATH solution
	const char *compare_checksums = "SELECT a.relative_path "
	        "FROM db2.files AS a "
	        "INNER JOIN db1.files AS b ON b.relative_path = a.relative_path "
	        "WHERE b.sha512 IS NOT a.sha512 "
	        "ORDER BY a.relative_path ASC;";
#endif

	/* Compare SHA512 checksums */
	sqlite3_stmt *select_stmt = NULL;
	bool first_iteration = true;

	if(verify_checksum_consistency == true)
	{
		if(SUCCESS == status)
		{
			int rc = sqlite3_prepare_v2(config->db,
				compare_checksums,
				-1,
				&select_stmt,
				NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement");
				status = FAILURE;
			}

			if(SUCCESS == status)
			{
				while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
				{
					checksum_mismatches_found = true;

					// Interrupt the loop smoothly
					// Interrupt when Ctrl+C
					if(global_interrupt_flag == true)
					{
						break;
					}

					if(first_iteration == true)
					{
						first_iteration = false;
						slog(EVERY,BOLD "The SHA512 checksums of these files do not match between %s and %s" RESET "\n",
							config->db_file_names[0],
							config->db_file_names[1]);
					}

		#if 0
					const unsigned char *relative_path = NULL;
					const unsigned char *path_prefix = NULL;
					path_prefix = sqlite3_column_text(select_stmt,0);
					relative_path = sqlite3_column_text(select_stmt,1);
		#endif

					const unsigned char *relative_path = sqlite3_column_text(select_stmt,0);

					if(relative_path != NULL)
					{
						slog(EVERY|UNDECOR,"%s\n",relative_path);
					} else {
						rc = sqlite3_errcode(config->db);
						log_sqlite_error(config->db,rc,NULL,"Failed to read relative path from select result");
						status = FAILURE;
						break;
					}
				}

				if(SUCCESS == status && global_interrupt_flag == false && SQLITE_DONE != rc)
				{
					log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
					status = FAILURE;
				}
			}
		}
	}

	/* Cleanup */
	if(attached_db2 == true)
	{
		call(db_finalize(config->db,"db2",&select_stmt));
	}

	if(attached_db1 == true)
	{
		sqlite3_stmt *no_stmt = NULL;
		call(db_finalize(config->db,"db1",&no_stmt));

		/* Detach databases in attach order */
		call(db_detach("db1"));
	}

	if(attached_db2 == true)
	{
		call(db_detach("db2"));
	}

	/* Output results */
	if(SUCCESS == status)
	{
		const bool full_compare_scope = check_first_source_only == true
		        && check_second_source_only == true
		        && verify_checksum_consistency == true;

		if(full_compare_scope == true
		        && first_source_only_differences_found == false
		        && second_source_only_differences_found == false
		        && checksum_mismatches_found == false)
		{
			slog(EVERY,BOLD "All files are identical against %s and %s" RESET "\n",
				config->db_file_names[0],
				config->db_file_names[1]);
		} else if(full_compare_scope == false){
			if(check_first_source_only == true
			        && first_source_only_differences_found == false)
			{
				slog(EVERY,BOLD "No first-source-only differences found between %s and %s" RESET "\n",
					config->db_file_names[0],
					config->db_file_names[1]);
			}

			if(check_second_source_only == true
			        && second_source_only_differences_found == false)
			{
				slog(EVERY,BOLD "No second-source-only differences found between %s and %s" RESET "\n",
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
		        && first_source_only_differences_found == false
		        && second_source_only_differences_found == false
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
