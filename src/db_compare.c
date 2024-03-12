#include "precizer.h"

/**
 *
 * @brief Compare two databases
 * @details The paths to both databases were passed as arguments
 * and stored in the Config structure
 *
 */
Return db_compare(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(config->compare != true){
		return(status);
	}

	slog(false,"Comparison of databases %s and %s is starting...\n",config->filenames[0],config->filenames[1]);

	/*
	 * Check up the integrity of database files
	 */

	// First database
	if(SUCCESS != (status = db_test(config->filenames[0])))
	{
		return(status);
	}

	// Second database
	if(SUCCESS != (status = db_test(config->filenames[1])))
	{
		return(status);
	}

	bool the_databases_are_equal = true;

	sqlite3_stmt *select_stmt = NULL;
	int rc = 0;

	// Compose a string with SQL request
	const char *attach_sql_1a = "attach database '";
	const char *attach_sql_1b = "' as db1;";
	char *select_sql_1 = (char *)calloc(strlen(attach_sql_1a) +
	                                  strlen(attach_sql_1b) +
	                                  strlen(config->databases_to_compare[0]) + 1,
	                                  sizeof(char));
	if(select_sql_1 == NULL)
	{
		status = FAILURE;
		slog(false,"ERROR: Memory allocation did not complete successfully!\n");
		return(status);
	}

	strcat(select_sql_1,attach_sql_1a);
	strcat(select_sql_1,config->databases_to_compare[0]);
	strcat(select_sql_1,attach_sql_1b);

	rc = sqlite3_exec(config->db, select_sql_1, NULL, NULL, NULL);
	if(rc!= SQLITE_OK ){
		slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	free(select_sql_1);

	// Compose a string with SQL request
	const char *attach_sql_2a = "attach database '";
	const char *attach_sql_2b = "' as db2;";
	char *select_sql_2 = (char *)calloc(strlen(attach_sql_2a) +
	                                  strlen(attach_sql_2b) +
	                                  strlen(config->databases_to_compare[1]) + 1,
	                                  sizeof(char));
	if(select_sql_2 == NULL)
	{
		status = FAILURE;
		slog(false,"ERROR: Memory allocation did not complete successfully!\n");
		return(status);
	}
	strcat(select_sql_2,attach_sql_2a);
	strcat(select_sql_2,config->databases_to_compare[1]);
	strcat(select_sql_2,attach_sql_2b);

	rc = sqlite3_exec(config->db, select_sql_2, NULL, NULL, NULL);
	if(rc!= SQLITE_OK ){
		slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	free(select_sql_2);


	const char *compare_A_sql = "SELECT a.relative_path " \
	                            "FROM db2.files AS a " \
	                            "LEFT JOIN db1.files AS b on b.relative_path = a.relative_path " \
	                            "WHERE b.relative_path IS NULL " \
	                            "ORDER BY a.relative_path ASC;";

	rc = sqlite3_prepare_v2(config->db, compare_A_sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	bool first_iteration = true;
	bool files_the_same = true;

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		the_databases_are_equal = false;
		files_the_same          = false;

		// Interrupt the loop smoothly
		// Interrupt when Ctrl+C
		if(global_interrupt_flag == true){
			break;
		}
		if (first_iteration == true){
			first_iteration = false;
			printf("\033[1mThese files no longer exist against %s but still present against %s\n\033[m",config->filenames[0],config->filenames[1]);
		}

		const unsigned char *relative_path = NULL;
		relative_path = sqlite3_column_text(select_stmt,0);

		if(relative_path != NULL){
			printf("%s\n",relative_path);
		} else {
			slog(false,"General database error!\n");
			status = FAILURE;
			break;
		}
	}
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(select_stmt);

	const char *compare_B_sql = "SELECT a.relative_path " \
	                            "FROM db1.files AS a " \
	                            "LEFT join db2.files AS b on b.relative_path = a.relative_path " \
	                            "WHERE b.relative_path IS NULL " \
	                            "ORDER BY a.relative_path ASC;";

	rc = sqlite3_prepare_v2(config->db, compare_B_sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	first_iteration = true;

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		the_databases_are_equal = false;
		files_the_same          = false;

		// Interrupt the loop smoothly
		// Interrupt when Ctrl+C
		if(global_interrupt_flag == true){
			break;
		}
		if (first_iteration == true){
			first_iteration = false;
			printf("\033[1mThese files no longer exist against %s but still present against %s\n\033[m",config->filenames[1],config->filenames[0]);
		}

		const unsigned char *relative_path = NULL;
		relative_path = sqlite3_column_text(select_stmt,0);

		if(relative_path != NULL){
			printf("%s\n",relative_path);
		} else {
			slog(false,"General database error!\n");
			status = FAILURE;
			break;
		}
	}
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

#if 0 // Old multiPATH solutions
	const char *compare_checksums = "select a.relative_path from db2.files a inner join db1.files b" \
	                                " on b.relative_path = a.relative_path " \
	                                " and b.sha512 != a.sha512" \
	                                " order by a.relative_path asc;";
#endif
#if 0
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

	rc = sqlite3_prepare_v2(config->db, compare_checksums, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	first_iteration = true;

	bool checksums = true;

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		the_databases_are_equal = false;
		checksums               = false;

		// Interrupt the loop smoothly
		// Interrupt when Ctrl+C
		if(global_interrupt_flag == true){
			break;
		}
		if (first_iteration == true){
			first_iteration = false;
			printf("\033[1mThe SHA512 checksums of these files do not match between %s and %s\n\033[m",config->filenames[0],config->filenames[1]);
		}

#if 0
		const unsigned char *relative_path = NULL;
		const unsigned char *path_prefix = NULL;
		path_prefix = sqlite3_column_text(select_stmt,0);
		relative_path = sqlite3_column_text(select_stmt,1);
#endif

		const unsigned char *relative_path = NULL;
		relative_path = sqlite3_column_text(select_stmt,0);

		if(relative_path != NULL){
			printf("%s\n",relative_path);
		} else {
			slog(false,"General database error!\n");
			status = FAILURE;
			break;
		}
	}
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	if(files_the_same == true)
	{
		printf("\033[1mAll files are identical against %s and %s\n\033[m",config->filenames[0],config->filenames[1]);
	}
	if(checksums == true)
	{
		printf("\033[1mAll SHA512 checksums of files are identical against %s and %s\n\033[m",config->filenames[0],config->filenames[1]);
	}

	if(the_databases_are_equal == true)
	{
		printf("\033[1mThe databases %s and %s are absolutely equal\033[m\n",config->filenames[0],config->filenames[1]);
	}

	return(status);
}
