#include "precizer.h"

/**
 * @brief Checks if the database exists and contains data
 *
 * @details This function verifies if the database has been previously created
 * and contains any paths data. If the database exists and the update flag
 * is not set, it warns the user about using the --update option.
 *
 * The function will skip the check if comparison mode is enabled.
 *
 * @note The function uses the global config structure to access database
 * settings and flags.
 *
 * @return Return enum value:
 *         - SUCCESS: Check completed successfully
 *         - FAILURE: Error occurred during check
 */
Return db_contains_data(void){
	/** @var Return status
	 *  @brief The status that will be passed to return() before exiting
	 *  @details By default, the function worked without errors
	 */
	Return status = SUCCESS;

	/* Skip check if in comparison mode */
	if(config->compare == true)
	{
		slog(TRACE,"Comparison mode is enabled. Main database verification is not required\n");
		return(status);
	}

	/** @var sqlite3_stmt *select_stmt
	 *  @brief SQLite prepared statement for counting paths
	 */
	sqlite3_stmt *select_stmt = NULL;
	int rc                    = 0;

	/* Initialize existence flag */
	config->db_contains_data = false;

	/** @var const char *sql_db_contains_data
	 *  @brief SQL query to count rows in paths table
	 */
	const char *sql_db_contains_data = "SELECT COUNT(*) FROM paths";

	/* Prepare the SQL statement */
	rc = sqlite3_prepare_v2(config->db,sql_db_contains_data,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Can't prepare select statement (%i): %s\n",rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		/* Execute the query and process results */
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			sqlite3_int64 rows = -1;
			rows = sqlite3_column_int64(select_stmt,0);

			if(rows > 0)
			{
				config->db_contains_data = true;
			}
		}

		/* Check for query execution errors */
		if(SQLITE_DONE != rc)
		{
			slog(ERROR,"Select statement didn't finish with DONE (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}
	sqlite3_finalize(select_stmt);

	if(SUCCESS == status)
	{
		/* Handle existing database case */
		if(config->db_contains_data == true)
		{
			if(config->update == true)
			{
				slog(TRACE,"The database %s has already been created previously\n",config->db_file_name);
			} else {
				slog(EVERY,"The database %s was previously created and already contains data with files and their checksums." \
					" Use the " BOLD "--update" RESET " option only when you are certain" \
					" that the database needs to be updated and when file information" \
					" (including changes, deletions, and additions) should be synchronized" \
					" with the database.\n",config->db_file_name);
				status = FAILURE;
			}
		}
	}

	return(status);
}
