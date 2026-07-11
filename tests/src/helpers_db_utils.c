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
Return open_db_from_tmpdir(
	const char *db_filename,
	const int  open_flags,
	sqlite3    **db_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,db_path,MEMORY_STRING);

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

		if(SQLITE_OK != sqlite3_open_v2(m_text(db_path),db_out,open_flags,NULL))
		{
			status = FAILURE;

			(void)sqlite3_close(*db_out);
			*db_out = NULL;
		}
	}

	m_del(db_path);

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
	m_create(char,db_path,MEMORY_STRING);

	if(SUCCESS == status && (db_filename == NULL || expected_paths == NULL || expected_count < 0))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(m_text(db_path),&db,SQLITE_OPEN_READONLY,NULL))
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	m_del(db_path);

	return(status);
}

/**
 * @brief Check whether one files-table row exists by relative path
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] exists_out Output existence flag
 *
 * @return Return status code:
 *         - SUCCESS: Query completed and output was filled
 *         - FAILURE: Validation, DB access, bind, or query execution failed
 */
Return db_relative_path_exists(
	const char *db_filename,
	const char *relative_path,
	bool       *exists_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT 1 FROM files WHERE relative_path = ?1 LIMIT 1;";

	if(db_filename == NULL || relative_path == NULL || exists_out == NULL)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		const int rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW)
		{
			*exists_out = true;

			if(SQLITE_DONE != sqlite3_step(stmt))
			{
				status = FAILURE;
			}

		} else if(rc == SQLITE_DONE){
			*exists_out = false;

		} else {
			status = FAILURE;
		}
	}

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Read files.ID for one row selected by relative path
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] id_out Output primary key value
 *
 * @return Return status code:
 *         - SUCCESS: Exactly one row was found and ID was read
 *         - FAILURE: Validation, DB access, missing row, or duplicate row failed
 */
Return db_read_file_id(
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *id_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT ID FROM files WHERE relative_path = ?1;";

	if(db_filename == NULL || relative_path == NULL || id_out == NULL)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW)
		{
			*id_out = sqlite3_column_int64(stmt,0);
			rc = sqlite3_step(stmt);

			if(rc != SQLITE_DONE)
			{
				status = FAILURE;
			}

		} else {
			status = FAILURE;
		}
	}

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Read first row ID from files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] row_id_out Output row ID
 *
 * @return Return status code:
 *         - SUCCESS: First row ID was read
 *         - FAILURE: Validation, DB access, or query execution failed
 */
Return db_read_first_row_id(
	const char    *db_filename,
	sqlite3_int64 *row_id_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT ID FROM files ORDER BY ID ASC LIMIT 1;";

	if(db_filename == NULL || row_id_out == NULL)
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
			*row_id_out = sqlite3_column_int64(stmt,0);
			rc = sqlite3_step(stmt);

			if(SQLITE_DONE != rc)
			{
				status = FAILURE;
			}
		} else {
			status = FAILURE;
		}
	}

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Overwrite stat blob for a specific files row ID
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] row_id Row ID in files table
 * @param[in] blob New blob bytes
 * @param[in] blob_size Size of blob in bytes
 *
 * @return Return status code:
 *         - SUCCESS: Blob was updated
 *         - FAILURE: Validation, DB access, bind, step, or change check failed
 */
Return db_overwrite_stat_blob_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	const void          *blob,
	const int           blob_size)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE files SET stat = ?1 WHERE ID = ?2;";

	if(db_filename == NULL || blob == NULL || blob_size < 0)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_blob(stmt,1,blob,blob_size,SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_int64(stmt,2,row_id))
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Corrupt stat blob for first files row with one-byte payload
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[out] row_id_out Optional output for affected row ID
 *
 * @return Return status code:
 *         - SUCCESS: First row stat blob was corrupted
 *         - FAILURE: Row lookup or blob overwrite failed
 */
Return db_corrupt_first_row_stat_blob(
	const char    *db_filename,
	sqlite3_int64 *row_id_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_int64 row_id = 0;
	const unsigned char corrupt_blob[] = {0xA5};

	if(SUCCESS == status)
	{
		status = db_read_first_row_id(db_filename,&row_id);
	}

	if(SUCCESS == status)
	{
		status = db_overwrite_stat_blob_by_row_id(db_filename,row_id,corrupt_blob,(int)sizeof(corrupt_blob));
	}

	if(SUCCESS == status && row_id_out != NULL)
	{
		*row_id_out = row_id;
	}

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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Read number of files rows whose stat blob has exact size
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] blob_size Expected stat blob size
 * @param[out] count_out Output row count
 *
 * @return Return status code:
 *         - SUCCESS: Count value was read
 *         - FAILURE: Validation, DB access, bind, or query execution failed
 */
Return db_read_files_count_with_blob_size(
	const char *db_filename,
	const int  blob_size,
	int        *count_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT COUNT(*) FROM files WHERE length(stat) = ?1;";

	if(db_filename == NULL || blob_size < 0 || count_out == NULL)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_int(stmt,1,blob_size))
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Update db_version value in metadata table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] db_version Version value to store
 *
 * @return Return status code:
 *         - SUCCESS: Version value was updated
 *         - FAILURE: Validation, DB access, bind, step, or change check failed
 */
Return set_db_version_in_metadata(
	const char *db_filename,
	const int  db_version)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE metadata SET db_version = ?1;";

	if(db_filename == NULL || db_version < 0)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_int(stmt,1,db_version))
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Read raw stat blob for a specific files row ID
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] row_id Row ID in files table
 * @param[out] blob_out Optional output buffer for blob bytes
 * @param[in] blob_out_size Size of blob_out buffer in bytes
 * @param[out] blob_size_out Output blob size from DB
 *
 * @return Return status code:
 *         - SUCCESS: Blob size and optional bytes were read
 *         - FAILURE: Validation, DB access, bind, or row parsing failed
 */
Return db_read_stat_blob_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	unsigned char       *blob_out,
	const size_t        blob_out_size,
	int                 *blob_size_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT stat FROM files WHERE ID = ?1;";

	if(db_filename == NULL || blob_size_out == NULL)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_int64(stmt,1,row_id))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(SQLITE_ROW == rc)
		{
			const void *blob = sqlite3_column_blob(stmt,0);
			const int blob_size = sqlite3_column_bytes(stmt,0);

			if(blob_size < 0)
			{
				status = FAILURE;
			}

			if(SUCCESS == status && blob_size > 0 && blob == NULL)
			{
				status = FAILURE;
			}

			if(SUCCESS == status && blob_out != NULL)
			{
				if((size_t)blob_size > blob_out_size)
				{
					status = FAILURE;
				} else if(blob_size > 0){
					memcpy(blob_out,blob,(size_t)blob_size);
				}
			}

			if(SUCCESS == status)
			{
				*blob_size_out = blob_size;
			}

			rc = sqlite3_step(stmt);

			if(SUCCESS == status && SQLITE_DONE != rc)
			{
				status = FAILURE;
			}
		} else {
			status = FAILURE;
		}
	}

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Read CmpctStat struct from files row by ID
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] row_id Row ID in files table
 * @param[out] stat_out Output compact stat structure
 *
 * @return Return status code:
 *         - SUCCESS: CmpctStat value was read
 *         - FAILURE: Validation, blob read, size check, or conversion failed
 */
Return db_read_cmpctstat_by_row_id(
	const char          *db_filename,
	const sqlite3_int64 row_id,
	CmpctStat           *stat_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	unsigned char raw[sizeof(CmpctStat)];
	int blob_size = 0;

	if(stat_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = db_read_stat_blob_by_row_id(db_filename,row_id,raw,sizeof(raw),&blob_size);
	}

	if(SUCCESS == status && blob_size != (int)sizeof(CmpctStat))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		memcpy(stat_out,raw,sizeof(CmpctStat));
	}

	return(status);
}

/**
 * @brief Read CmpctStat blob for one file by relative path
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[out] stat_out Output compact stat structure
 *
 * @return Return status code:
 *         - SUCCESS: CmpctStat value was read
 *         - FAILURE: Validation, DB access, blob size check, or row parsing failed
 */
Return db_read_cmpctstat_by_relative_path(
	const char *db_filename,
	const char *relative_path,
	CmpctStat  *stat_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT stat FROM files WHERE relative_path = ?1;";

	if(db_filename == NULL || relative_path == NULL || stat_out == NULL)
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

	if(SUCCESS == status && SQLITE_OK != sqlite3_bind_text(stmt,1,relative_path,(int)strlen(relative_path),SQLITE_TRANSIENT))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		int rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW)
		{
			const void *blob = sqlite3_column_blob(stmt,0);
			const int bytes = sqlite3_column_bytes(stmt,0);

			if(blob == NULL || bytes != (int)sizeof(CmpctStat))
			{
				status = FAILURE;
			} else {
				memcpy(stat_out,blob,sizeof(CmpctStat));
			}

			rc = sqlite3_step(stmt);

			if(SUCCESS == status && rc != SQLITE_DONE)
			{
				status = FAILURE;
			}
		} else {
			status = FAILURE;
		}
	}

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Check whether DB compact-stat timestamps match current file stat timestamps
 *
 * @param[in] db_stat Compact stat loaded from the database
 * @param[in] file_stat Current filesystem stat structure
 * @return `true` when both ctime and mtime fields match exactly, otherwise `false`
 */
bool cmpctstat_matches_stat_timestamps(
	const CmpctStat   *db_stat,
	const struct stat *file_stat)
{
	if(db_stat == NULL || file_stat == NULL)
	{
		return(false);
	}

	return(db_stat->mtim_tv_sec == file_stat->st_mtim.tv_sec &&
	       db_stat->mtim_tv_nsec == file_stat->st_mtim.tv_nsec &&
	       db_stat->ctim_tv_sec == file_stat->st_ctim.tv_sec &&
	       db_stat->ctim_tv_nsec == file_stat->st_ctim.tv_nsec);
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
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *offset_out,
	unsigned char *sha512_out)
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Verify that a final DB checksum matches the file on disk
 *
 * Reads the application's stored SHA512 digest, requires the final offset state
 * to be clear, computes the same file with the Monocypher oracle, and compares
 * the raw digest bytes
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 * @param[in] file_path Absolute path to the file being checked
 *
 * @return Return status code:
 *         - SUCCESS: Stored final checksum matches the file
 *         - FAILURE: Validation, DB read, non-final offset, hash, or compare failed
 */
Return db_final_sha512_matches_file(
	const char *db_filename,
	const char *relative_path,
	const char *file_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_int64 final_offset = -1;
	unsigned char db_sha512[SHA512_DIGEST_LENGTH] = {0};
	unsigned char expected_sha512[SHA512_DIGEST_LENGTH] = {0};

	if(db_filename == NULL || relative_path == NULL || file_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = read_final_sha512_from_db(db_filename,relative_path,&final_offset,db_sha512);
	}

	if(SUCCESS == status && final_offset != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = compute_file_sha512_monocypher(file_path,expected_sha512);
	}

	if(SUCCESS == status && memcmp(db_sha512,expected_sha512,(size_t)SHA512_DIGEST_LENGTH) != 0)
	{
		status = FAILURE;
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	return(status);
}

/**
 * @brief Corrupt stored SHA512 bytes for one files-table row
 *
 * The row is selected by relative_path and the SHA512 blob is modified in a
 * deterministic way while keeping the SQL value non-NULL. This is useful for
 * tests that need a database checksum mismatch without changing fixture files
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 *
 * @return Return status code:
 *         - SUCCESS: SHA512 value was updated for at least one row
 *         - FAILURE: Validation, DB access, bind, step, or change check failed
 */
Return db_tamper_sha512(
	const char *db_filename,
	const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE files SET sha512 = (substr(sha512,1,2) || X'BEEF' || substr(sha512,5)) "
	        "WHERE relative_path = ?1;";

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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

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
	const char    *db_filename,
	const char    *relative_path,
	sqlite3_int64 *offset_out,
	int           *md_context_bytes_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT offset, mdContext FROM files WHERE relative_path = ?1;";
	m_create(char,db_path,MEMORY_STRING);

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

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(m_text(db_path),&db,SQLITE_OPEN_READONLY,NULL))
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

	(void)sqlite3_finalize(stmt);

	(void)sqlite3_close(db);

	m_del(db_path);

	return(status);
}

/**
 * @brief Verify that a files-table row has no partial resume state
 *
 * A completed hash must have a cleared offset and an empty mdContext blob.
 * This helper intentionally treats any non-empty value as a failure because it
 * would make a future update look resumable
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 *
 * @return Return status code:
 *         - SUCCESS: Offset and mdContext are both clear
 *         - FAILURE: Validation, DB read, non-zero offset, or non-empty context failed
 */
Return db_resume_state_is_empty(
	const char *db_filename,
	const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_int64 offset = -1;
	int md_context_bytes = -1;

	if(db_filename == NULL || relative_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = read_resume_state_from_db(db_filename,relative_path,&offset,&md_context_bytes);
	}

	if(SUCCESS == status && offset != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && md_context_bytes != 0)
	{
		status = FAILURE;
	}

	return(status);
}
