#include "sute.h"

/**
 * @brief Verify that DB relative_path set matches expected list exactly
 *
 * @param[in] db_filename DB file name relative to TMPDIR
 * @param[in] expected_paths Sorted expected relative_path values
 * @param[in] expected_count Number of expected paths
 *
 * @return Return status code:
 *         - SUCCESS: DB rows match expected paths exactly
 *         - FAILURE: Mismatch or DB access error
 */
Return db_paths_match(
	const char        *db_filename,
	const char *const *expected_paths,
	const int         expected_count)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT relative_path FROM files ORDER BY relative_path ASC;";
	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL || expected_paths == NULL || expected_count < 0))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READONLY,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	int index = 0;

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		while(rc == SQLITE_ROW)
		{
			if(index >= expected_count)
			{
				status = FAILURE;
				break;
			}

			const unsigned char *db_path_text = sqlite3_column_text(stmt,0);

			if(db_path_text == NULL || strcmp((const char *)db_path_text,expected_paths[index]) != 0)
			{
				status = FAILURE;
				break;
			}

			index++;
			rc = sqlite3_step(stmt);
		}

		if(SUCCESS == status && rc != SQLITE_DONE)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && index != expected_count)
	{
		status = FAILURE;
	}

	if(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	if(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	del(db_path);

	return(status);
}
