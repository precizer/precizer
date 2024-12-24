#include "precizer.h"

/**
 * @brief Composes SQL ATTACH DATABASE query string
 * @param[out] sql Pointer to string that will hold the SQL query
 * @param[in] db_path Path to database file
 * @param[in] db_num Database number (1 or 2)
 * @return Return structure containing operation status
 */
static Return compose_sql(
	char       **sql,
	const char *db_path,
	int        db_num
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(asprintf(sql,"ATTACH DATABASE '%s' as db%d;",db_path,db_num) == -1)
	{
		status = FAILURE;
		report("Memory allocation failed for SQL query string");
	}

	return(status);
}

static Return db_attach(
	int db_A,
	int db_B
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	char *select_sql = NULL;
	int rc = 0;

	run(compose_sql(&select_sql,config->db_file_paths[db_A],db_B));

	if(SUCCESS == status)
	{
		rc = sqlite3_exec(config->db,select_sql,NULL,NULL,NULL);

		if(rc!= SQLITE_OK)
		{
			slog(ERROR,"Can't execute (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	free(select_sql);

	return(status);
}


static Return db_changes(
	const char *compare_sql,
	bool       *files_the_same,
	bool       *the_databases_are_equal,
	int        db_A,
	int        db_B
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	bool first_iteration = true;

	sqlite3_stmt *select_stmt = NULL;

	int rc = sqlite3_prepare_v2(config->db,compare_sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Can't prepare select statement (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		*the_databases_are_equal = false;
		*files_the_same = false;

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
			slog(EVERY,"%s\n",relative_path);
		} else {
			slog(ERROR,"General database error!\n");
			status = FAILURE;
			break;
		}
	}

	if(SQLITE_DONE != rc)
	{
		slog(ERROR,"Select statement didn't finish with DONE (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(select_stmt);

	return(status);
}

/**
 *
 * @brief Compare two databases
 * @details The paths to both databases were passed as arguments
 * and stored in the Config structure
 *
 */
Return db_compare(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(config->compare != true)
	{
		slog(TRACE,"Database comparison mode is not enabled. Skipping comparison\n");
		return(status);
	}

	slog(EVERY,"The comparison of %s and %s databases is starting…\n",config->db_file_names[0],config->db_file_names[1]);

	/*
	 *
	 * Validate the path availability
	 *
	 */

	// Validate the paths passed as arguments and make sure
	// that they are files and exist
	for(int i = 0; config->db_file_paths[i]; i++)
	{
		if(NOT_FOUND == file_availability(config->db_file_paths[i],SHOULD_BE_A_FILE))
		{
			// The path doesn't exist or is not a database
			slog(ERROR,"The database file %s is either inaccessible or not a valid file\n",config->db_file_paths[i]);
			return(FAILURE);
		}

		/*
		 * Validate the integrity of database files
		 */
		if(SUCCESS != (status = db_test(config->db_file_paths[i])))
		{
			return(status);
		}
	}

	// Attache the database 1
	run(db_attach(0,1));

	// Attache the database 2
	run(db_attach(1,2));

	const char *compare_A_sql = "SELECT a.relative_path " \
	        "FROM db2.files AS a " \
	        "LEFT JOIN db1.files AS b on b.relative_path = a.relative_path " \
	        "WHERE b.relative_path IS NULL " \
	        "ORDER BY a.relative_path ASC;";

	const char *compare_B_sql = "SELECT a.relative_path " \
	        "FROM db1.files AS a " \
	        "LEFT join db2.files AS b on b.relative_path = a.relative_path " \
	        "WHERE b.relative_path IS NULL " \
	        "ORDER BY a.relative_path ASC;";

	bool files_the_same = true;
	bool the_databases_are_equal = true;

	run(db_changes(compare_A_sql,&files_the_same,&the_databases_are_equal,0,1));
	run(db_changes(compare_B_sql,&files_the_same,&the_databases_are_equal,1,0));

#if 0 // Old multiPATH solutions
	const char *compare_checksums = "select a.relative_path from db2.files a inner join db1.files b" \
	        " on b.relative_path = a.relative_path " \
	        " and b.sha512 != a.sha512" \
	        " order by a.relative_path asc;";

	const char *compare_checksums = "SELECT p.path,f1.relative_path " \
	        "FROM db1.files AS f1 " \
	        "JOIN db1.paths AS p ON f1.path_prefix_index = p.ID " \
	        "JOIN db2.files AS f2 ON f1.relative_path = f2.relative_path " \
	        "JOIN db2.paths AS p2 ON f2.path_prefix_index = p2.ID " \
	        "WHERE f1.sha512 <> f2.sha512 AND p.path = p2.path " \
	        "ORDER BY p.path,f1.relative_path ASC;";
#endif

	// One PATH solution
	const char *compare_checksums = "SELECT a.relative_path " \
	        "FROM db2.files AS a " \
	        "INNER JOIN db1.files b on b.relative_path = a.relative_path and b.sha512 != a.sha512 " \
	        "ORDER BY a.relative_path ASC;";

	sqlite3_stmt *select_stmt = NULL;

	int rc = sqlite3_prepare_v2(config->db,compare_checksums,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Can't prepare select statement (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	bool first_iteration = true;

	bool checksums = true;

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		the_databases_are_equal = false;
		checksums = false;

		// Interrupt the loop smoothly
		// Interrupt when Ctrl+C
		if(global_interrupt_flag == true)
		{
			break;
		}

		if(first_iteration == true)
		{
			first_iteration = false;
			slog(EVERY,BOLD "The SHA512 checksums of these files do not match between %s and %s" RESET "\n",config->db_file_names[0],config->db_file_names[1]);
		}

#if 0
		const unsigned char *relative_path = NULL;
		const unsigned char *path_prefix = NULL;
		path_prefix = sqlite3_column_text(select_stmt,0);
		relative_path = sqlite3_column_text(select_stmt,1);
#endif

		const unsigned char *relative_path = NULL;
		relative_path = sqlite3_column_text(select_stmt,0);

		if(relative_path != NULL)
		{
			slog(EVERY,"%s\n",relative_path);
		} else {
			slog(ERROR,"General database error!\n");
			status = FAILURE;
			break;
		}
	}

	if(SQLITE_DONE != rc)
	{
		slog(ERROR,"Select statement didn't finish with DONE (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	if(files_the_same == true)
	{
		slog(EVERY,BOLD "All files are identical against %s and %s" RESET "\n",config->db_file_names[0],config->db_file_names[1]);
	}

	if(checksums == true)
	{
		slog(EVERY,BOLD "All SHA512 checksums of files are identical against %s and %s" RESET "\n",config->db_file_names[0],config->db_file_names[1]);
	}

	if(the_databases_are_equal == true)
	{
		slog(EVERY,BOLD "The databases %s and %s are absolutely equal" RESET "\n",config->db_file_names[0],config->db_file_names[1]);
	}

	slog(EVERY,"Comparison of %s and %s databases is complete\n",config->db_file_names[0],config->db_file_names[1]);

	return(status);
}
