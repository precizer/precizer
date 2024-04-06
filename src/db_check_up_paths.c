#include "precizer.h"

/**
 *
 * The function checks those paths that have already been written
 * to the database and compares them with the paths passed
 * as arguments. If a mismatch is detected, the program stops
 * to avoid accidental deletion of data from the database
 *
*/
Return db_check_up_paths(void)
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
	sqlite3_stmt *insert_stmt = NULL;
	int rc = 0;

	char *select_sql = NULL;
	bool paths_are_equal = true;

	// Create the SQL request
	if(config->paths[0] != NULL)
	{
		char const *sql_1 = "SELECT ID FROM paths WHERE prefix NOT IN (";
		size_t size = strlen(sql_1) + 1;
		select_sql = (char *)calloc(size,sizeof(char));
		if(select_sql == NULL)
		{
			slog(false,"ERROR: Memory allocation did not complete successfully!\n");
			status = FAILURE;
			return(status);
		}
		strcat(select_sql,sql_1);

		for (int i = 0; config->paths[i]; i++)
		{
			// Not the last
			if(config->paths[i+1] != 0)
			{
				// Size of comma
				size += 1;
			}
			char *prefix = config->paths[i];
			size += strlen(prefix) + 2; // Length of the line and two chars like '
			char *tmp = (char *)realloc(select_sql,size);
			if(NULL == tmp)
			{
				slog(false,"Realloc error\n");
				free(select_sql);
				status = FAILURE;
				return(status);
			} else {
				select_sql = tmp;
			}
			strcat(select_sql,"'");
			strcat(select_sql,prefix);
			strcat(select_sql,"'");

			// Not the last
			if(config->paths[i+1] != 0)
			{
				// Add comma
				strcat(select_sql,",");
			}
		}

		// Close the string that contains SQL request
		char const *sql_2 = ");";
		size += strlen(sql_2);
		char *tmp = (char *)realloc(select_sql,size);
		if(NULL == tmp)
		{
			slog(false,"Realloc error\n");
			free(select_sql);
			status = FAILURE;
			return(status);
		} else {
			select_sql = tmp;
		}
		strcat(select_sql,sql_2);
	}

	rc = sqlite3_prepare_v2(config->db, select_sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statement %s (%i): %s\n", select_sql, rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		sqlite3_int64 path_ID = -1;

		path_ID = sqlite3_column_int64(select_stmt,0);

		if(path_ID != -1)
		{
			paths_are_equal = false;

			const char *insert_sql = "INSERT INTO runtime_paths_id.the_path_id_does_not_exists (path_id) VALUES (?1);";

			rc = sqlite3_prepare_v2(config->db, insert_sql, -1, &insert_stmt, NULL);
			if(SQLITE_OK != rc) {
				slog(false,"Can't prepare insert statement (%i): %s\n", rc, sqlite3_errmsg(config->db));
				status = FAILURE;
			}

			rc = sqlite3_bind_int64(insert_stmt,1,path_ID);
			if(SQLITE_OK != rc) {
				slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
				status = FAILURE;
			}

			/* Execute SQL statement */
			if(sqlite3_step(insert_stmt) != SQLITE_DONE)
			{
				slog(false,"Insert statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
				status = FAILURE;
			}
			sqlite3_finalize(insert_stmt);
		}
	}
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	free(select_sql);

	sqlite3_finalize(select_stmt);

	if(paths_are_equal == true)
	{
		if(config->verbose == true)
		{
			slog(false,"The paths written against the database and the paths passed as arguments are completely identical. Nothing will be lost\n");
		}
	} else {
		slog(false,"The paths passed as arguments are different with the saved against the database. Information about files and their checksums may be lost!\n");
		if(config->silent == false)
		{
			slog(false,"The paths saved against the database: ");

			sqlite3_stmt * stmt;
			int rc_stmt = 0;
			char const *sql = "SELECT prefix FROM paths;";

			rc_stmt = sqlite3_prepare_v2(config->db, sql, -1, &stmt, NULL);
			if(SQLITE_OK != rc_stmt) {
				slog(false,"Can't prepare select statement %s (%i): %s\n", sql, rc_stmt, sqlite3_errmsg(config->db));
				status = FAILURE;
			}

			bool first_iteration = true;

			while(SQLITE_ROW == (rc_stmt = sqlite3_step(stmt)))
			{
				const char *prefix = (const char *)sqlite3_column_text(stmt,0);
				if(first_iteration == true){
					printf("%s",prefix);
					first_iteration = false;
				} else {
					printf(", %s",prefix);
				}
			}
			if(SQLITE_DONE != rc_stmt) {
				slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc_stmt, sqlite3_errmsg(config->db));
				status = FAILURE;
			}
			printf("\n");

			sqlite3_finalize(stmt);
		}

		if(config->force == true){
			if(config->silent == false)
			{
				printf("The \033[1m--force\033[m option has been used, so the following path will be written to the database %s: ",config->db_file_name);
				for (int i = 0; config->paths[i]; i++)
				{
					printf (i == 0 ? "%s" : ", %s", config->paths[i]);
				}
				printf("\n");
			}
		} else {
			slog(false,"Use \033[1m--force\033[m option only in case when the PATH that was written into " \
			           "the database really need to be renewed. Warning! If this option will be used in incorrect way, " \
			           "information about files and their checksums against the database would " \
			           "be lost.\n");
			status = WARNING;
		}
	}

	return(status);
}
