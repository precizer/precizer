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

/**
 * @brief Read intermediate offset and mdContext size for one file from DB
 *
 * @param[in] db_filename DB file name relative to TMPDIR
 * @param[in] relative_path File path in DB relative_path column
 * @param[out] offset_out Output offset value from files table
 * @param[out] md_context_bytes_out Output byte size of mdContext column
 *
 * @return Return status code:
 *         - SUCCESS: Row was found and outputs were filled
 *         - FAILURE: Validation, DB access, or row parsing failed
 */
Return read_resume_state_from_db(
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	int            *md_context_bytes_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT offset, mdContext FROM files WHERE relative_path = ?1;";
	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL
	        || relative_path == NULL
	        || offset_out == NULL
	        || md_context_bytes_out == NULL))
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const int step_rc = sqlite3_step(stmt);
		if(step_rc != SQLITE_ROW)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(sqlite3_column_type(stmt,0) == SQLITE_NULL)
		{
			*offset_out = 0;
		} else {
			*offset_out = sqlite3_column_int64(stmt,0);
		}

		*md_context_bytes_out = sqlite3_column_bytes(stmt,1);

		const int done_rc = sqlite3_step(stmt);
		if(done_rc != SQLITE_DONE)
		{
			status = FAILURE;
		}
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
