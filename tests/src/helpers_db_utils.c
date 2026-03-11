#include "sute.h"

/**
 * @brief Open SQLite database from TMPDIR by relative filename
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] open_flags Flags passed to sqlite3_open_v2
 * @param[out] db_out Opened database handle
 *
 * @return Return status code:
 *         - SUCCESS: Database opened successfully
 *         - FAILURE: Validation, path construction, or open failed
 */
static Return open_db_from_tmpdir(
	const char *db_filename,
	const int  open_flags,
	sqlite3    **db_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	create(char,db_path);

	if(db_filename == NULL || db_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status)
	{
		*db_out = NULL;

		if(SQLITE_OK != sqlite3_open_v2(getcstring(db_path),db_out,open_flags,NULL))
		{
			status = FAILURE;

			if(*db_out != NULL)
			{
				(void)sqlite3_close(*db_out);
				*db_out = NULL;
			}
		}
	}

	del(db_path);

	return(status);
}

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
 * @brief Read number of rows from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] count_out Output row count
 *
 * @return Return status code:
 *         - SUCCESS: Count value was read
 *         - FAILURE: Validation, DB access, or query execution failed
 */
Return db_read_files_count(
	const char *db_filename,
	int        *count_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT COUNT(*) FROM files;";

	if(db_filename == NULL || count_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_db_from_tmpdir(db_filename,SQLITE_OPEN_READONLY,&db);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(SQLITE_ROW == rc)
		{
			*count_out = sqlite3_column_int(stmt,0);
			rc = sqlite3_step(stmt);

			if(SQLITE_DONE != rc)
			{
				status = FAILURE;
			}
		} else {
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

	return(status);
}

/**
 * @brief Read db_version value from metadata table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] db_version_out Output database version
 *
 * @return Return status code:
 *         - SUCCESS: Version value was read
 *         - FAILURE: Validation, DB access, or query execution failed
 */
Return read_db_version_from_metadata(
	const char *db_filename,
	int        *db_version_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT db_version FROM metadata LIMIT 1;";

	if(db_filename == NULL || db_version_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_db_from_tmpdir(db_filename,SQLITE_OPEN_READONLY,&db);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(SQLITE_ROW == rc)
		{
			*db_version_out = sqlite3_column_int(stmt,0);
			rc = sqlite3_step(stmt);

			if(SQLITE_DONE != rc)
			{
				status = FAILURE;
			}
		} else {
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

	return(status);
}

/**
 * @brief Read final offset and SHA512 checksum for one file from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] offset_out Output offset value, 0 when SQL value is NULL
 * @param[out] sha512_out Output SHA512 bytes with SHA512_DIGEST_LENGTH size
 *
 * @return Return status code:
 *         - SUCCESS: Row was found and outputs were filled
 *         - FAILURE: Validation, DB access, missing row, or SHA512 blob size mismatch
 */
Return read_final_sha512_from_db(
	const char     *db_filename,
	const char     *relative_path,
	sqlite3_int64  *offset_out,
	unsigned char  *sha512_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT offset, sha512 FROM files WHERE relative_path = ?1;";

	if(db_filename == NULL
	        || relative_path == NULL
	        || offset_out == NULL
	        || sha512_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_db_from_tmpdir(db_filename,SQLITE_OPEN_READONLY,&db);
	}

	if((SUCCESS == status) && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if((SUCCESS == status) && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
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

		const void *sha512_blob = sqlite3_column_blob(stmt,1);
		const int sha512_bytes = sqlite3_column_bytes(stmt,1);

		if(sha512_blob == NULL || sha512_bytes != SHA512_DIGEST_LENGTH)
		{
			status = FAILURE;
		} else {
			memcpy(sha512_out,sha512_blob,(size_t)SHA512_DIGEST_LENGTH);
		}

		const int done_rc = sqlite3_step(stmt);
		if((SUCCESS == status) && done_rc != SQLITE_DONE)
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

	return(status);
}

/**
 * @brief Set files.sha512 to NULL for one row in the database
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 *
 * @return Return status code:
 *         - SUCCESS: SHA512 value was set to NULL for at least one row
 *         - FAILURE: Validation, DB access, bind, step, or change check failed
 */
Return db_set_sha512_to_null(
	const char *db_filename,
	const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE files SET sha512 = NULL WHERE relative_path = ?1;";

	if(db_filename == NULL || relative_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_db_from_tmpdir(db_filename,SQLITE_OPEN_READWRITE,&db);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_prepare_v2(db,sql,-1,&stmt,NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_DONE != sqlite3_step(stmt))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && sqlite3_changes(db) < 1)
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
