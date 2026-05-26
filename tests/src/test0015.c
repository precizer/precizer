#include "sute.h"
#include "db_upgrade.h"

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

			IF(*db_out != NULL)
			{
				(void)sqlite3_close(*db_out);
				*db_out = NULL;
			}
		}
	}

	m_del(db_path);

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
static Return db_read_first_row_id(
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

	IF(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

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
static Return db_overwrite_stat_blob_by_row_id(
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

	IF(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

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
static Return db_read_files_count_with_blob_size(
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

	IF(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

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
static Return set_db_version_in_metadata(
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

	IF(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

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
static Return db_read_stat_blob_by_row_id(
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

	IF(stmt != NULL)
	{
		(void)sqlite3_finalize(stmt);
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

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
static Return db_read_cmpctstat_by_row_id(
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
 * @brief Validate zero-converted CmpctStat values after migration fixups
 *
 * @param[in] stat Compact stat structure to verify
 *
 * @return Return status code:
 *         - SUCCESS: Structure matches expected zero-converted values
 *         - FAILURE: One or more fields differ from expected values
 */
static Return db_verify_zero_converted_cmpctstat(
	const CmpctStat *stat)
{
	if(stat == NULL)
	{
		return FAILURE;
	}

	if(stat->st_size != 0)
	{
		return FAILURE;
	}

	if(stat->st_blocks != BLKCNT_UNKNOWN)
	{
		return FAILURE;
	}

	if(stat->st_dev != 0 || stat->st_ino != 0)
	{
		return FAILURE;
	}

	if(stat->mtim_tv_sec != 0 || stat->mtim_tv_nsec != 0)
	{
		return FAILURE;
	}

	if(stat->ctim_tv_sec != 0 || stat->ctim_tv_nsec != 0)
	{
		return FAILURE;
	}

	return SUCCESS;
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
static Return db_corrupt_first_row_stat_blob(
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
 * @brief Create trigger that aborts on second stat update in files table
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Trigger and helper table were created
 *         - FAILURE: Validation, DB open, or SQL execution failed
 */
static Return db_create_abort_on_second_stat_update_trigger(
	const char *db_filename)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	const char *sql =
	        "DROP TRIGGER IF EXISTS __test_abort_on_second_update;"
	        "DROP TABLE IF EXISTS __test_fail_counter;"
	        "CREATE TABLE __test_fail_counter(n INTEGER NOT NULL);"
	        "INSERT INTO __test_fail_counter(n) VALUES(0);"
	        "CREATE TRIGGER __test_abort_on_second_update "
	        "BEFORE UPDATE OF stat ON files "
	        "BEGIN "
	        "UPDATE __test_fail_counter SET n = n + 1;"
	        "SELECT CASE WHEN (SELECT n FROM __test_fail_counter LIMIT 1) >= 2 "
	        "THEN RAISE(ABORT,'forced rollback test failure') END;"
	        "END;";

	if(db_filename == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_db_from_tmpdir(db_filename,SQLITE_OPEN_READWRITE,&db);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_exec(db,sql,NULL,NULL,NULL))
	{
		status = FAILURE;
	}

	IF(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	return(status);
}

/**
 * @brief Reject using a legacy v0 DB as the primary database without --update
 *
 * Verify the run fails and prints the expected warning for the upgrade path
 */
Return test0015_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v0.db","0015_database_v0.db"));

	const char *arguments = "--database=./0015_database_v0.db tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_001.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v0.db"));

	RETURN_STATUS;
}

/**
 * @brief Upgrade a legacy v0 DB as the primary database with --update
 *
 * Verify the upgrade succeeds and produces the expected output
 */
Return test0015_2(void)
{
	INITTEST;
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v0.db","0015_database_v0.db"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=0015_database_v0.db "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_002_1.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v0.db"));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Upgrade a legacy v0 DB with --watch-timestamps enabled
 *
 * Verify the upgrade succeeds and reports timestamp details in the output
 */
Return test0015_3(void)
{
	INITTEST;
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v0.db","0015_database_v0.db"));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--watch-timestamps --update --database=0015_database_v0.db "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_002_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Re-run upgrade for the already upgraded v0 DB
 *
 * Verify the database is treated as current and the run stays successful
 */
Return test0015_4(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_003.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v0.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v0.db"));

	RETURN_STATUS;
}

/**
 * @brief Create a fresh DB with the default generated filename
 *
 * Verify the application reports the generated DB name in the expected output
 */
Return test0015_5(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Run the application without an explicit DB name
	const char *arguments = "tests/fixtures/diffs/diff1";

	const char *filename = "templates/0015_004.txt"; // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME"); // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	RETURN_STATUS;
}

/**
 * @brief Compare a current DB against a legacy v0 DB without --update
 *
 * Verify the application warns that the legacy DB must be upgraded first
 */
Return test0015_6(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v0.db","0015_database_v0.db"));

	const char *arguments = "--compare $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_005.txt"; // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME"); // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,WARNING));

	RETURN_STATUS;
}

/**
 * @brief Compare and upgrade a legacy v0 DB with --compare --update
 *
 * Verify the legacy DB is upgraded during comparison and the run completes
 */
Return test0015_7(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Run the comparison and upgrade flow
	const char *arguments = "--compare --update $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_006.txt"; // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME"); // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v0.db"));

	RETURN_STATUS;
}

/**
 * @brief Upgrade a legacy v1 DB as the primary database with --update
 *
 * Verify the upgrade succeeds and produces the expected output
 */
Return test0015_8(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v1.db","0015_database_v1.db"));

	const char *arguments = "--update --database=0015_database_v1.db "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_007.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Re-run upgrade for the already upgraded v1 DB
 *
 * Verify the database is treated as current and the run stays successful
 */
Return test0015_9(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *filename = "templates/0015_008.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v1.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v1.db"));

	RETURN_STATUS;
}

/**
 * @brief Compare and upgrade a legacy v1 DB with --compare --update
 *
 * Verify the legacy DB is upgraded during comparison and the run completes
 */
Return test0015_10(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Run the comparison and upgrade flow
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v1.db","0015_database_v1.db"));

	const char *arguments = "--compare --update $DBNAME 0015_database_v1.db";

	const char *filename = "templates/0015_009.txt"; // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME"); // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v1.db"));

	RETURN_STATUS;
}

/**
 * @brief Upgrade a legacy v2 DB as the primary database with --update
 *
 * Verify the upgrade succeeds and prints the verbose migration details
 */
Return test0015_11(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v2.db","0015_database_v2.db"));

	const char *arguments = "--update --database=0015_database_v2.db --verbose "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_010.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v2.db"));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Compare and upgrade a legacy v2 DB with --compare --update
 *
 * Verify the legacy DB is upgraded during comparison and the run completes
 */
Return test0015_12(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Run the comparison and upgrade flow
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v2.db","0015_database_v2.db"));

	const char *arguments = "--compare --update $DBNAME 0015_database_v2.db";

	const char *filename = "templates/0015_011.txt"; // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME"); // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	ASSERT(SUCCESS == delete_path(replacement));
	ASSERT(SUCCESS == delete_path("0015_database_v2.db"));

	RETURN_STATUS;
}

/**
 * @brief Upgrade a legacy v3 DB whose filename contains UTF-8 characters
 *
 * Verify the primary DB upgrade works with spaces and non-ASCII characters
 */
Return test0015_13(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db","0015_database_v3 это база данных с пробелами и символами UTF-8.db"));

	const char *arguments = "--update --database=\"0015_database_v3 это база данных с пробелами и символами UTF-8.db\" "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_012.txt";

	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	ASSERT(SUCCESS == delete_path("0015_database_v3 это база данных с пробелами и символами UTF-8.db"));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Compare and upgrade a legacy v3 UTF-8 DB with --compare --update
 *
 * Verify the upgrade path works when both compared DB filenames contain UTF-8
 */
Return test0015_14(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db","0015_database_v3 это база данных с пробелами и символами UTF-8.db"));

	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db","0015_database_v4 это база данных с пробелами и символами UTF-8.db"));

	const char *arguments = "--compare --update "
	        "\"0015_database_v3 это база данных с пробелами и символами UTF-8.db\" "
	        "\"0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_013.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	m_del(result);
	m_del(pattern);

	ASSERT(SUCCESS == delete_path("0015_database_v3 это база данных с пробелами и символами UTF-8.db"));
	ASSERT(SUCCESS == delete_path("0015_database_v4 это база данных с пробелами и символами UTF-8.db"));

	RETURN_STATUS;
}

/**
 * @brief Compare a fresh UTF-8 DB against a legacy UTF-8 v4 reference DB
 *
 * Verify the application can create, read, and compare DB filenames with
 * spaces and non-ASCII characters while keeping checksum compatibility
 */
Return test0015_15(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db","0015_database_v4 это база данных с пробелами и символами UTF-8.db"));

	m_create(char,pattern,MEMORY_STRING);
	m_create(char,result,MEMORY_STRING);
	m_create(char,chunk,MEMORY_STRING);

	const char *arguments = "--database=\"Это новая база данных.db\" "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_copy(result,chunk));

	arguments = "--compare \"Это новая база данных.db\" "
	        "\"0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	const char *filename = "templates/0015_014.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);
	m_del(chunk);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("Это новая база данных.db"));
	ASSERT(SUCCESS == delete_path("0015_database_v4 это база данных с пробелами и символами UTF-8.db"));

	RETURN_STATUS;
}

/**
 * @brief Upgrade a v0 DB with one corrupted legacy stat blob
 *
 * Verify the full upgrade still completes and the corrupted row is converted
 * into the expected zero-source compact stat representation
 */
Return test0015_16(void)
{
	INITTEST;
	const char *corrupted_db_filename = "0015_database_v0_corrupt.db";
	const char *reference_db_filename = "0015_database_v4_reference.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v0.db",corrupted_db_filename));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db",reference_db_filename));

	sqlite3_int64 row_id = 0;

	ASSERT(SUCCESS == db_corrupt_first_row_stat_blob(corrupted_db_filename,&row_id));

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	const char *arguments = "--compare --update 0015_database_v4_reference.db 0015_database_v0_corrupt.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0015_015.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata(corrupted_db_filename,&db_version));
	ASSERT(db_version == 4);

	CmpctStat stat = {0};
	ASSERT(SUCCESS == db_read_cmpctstat_by_row_id(corrupted_db_filename,row_id,&stat));
	ASSERT(SUCCESS == db_verify_zero_converted_cmpctstat(&stat));

	ASSERT(SUCCESS == delete_path(corrupted_db_filename));
	ASSERT(SUCCESS == delete_path(reference_db_filename));

	m_del(result);
	m_del(pattern);

	RETURN_STATUS;
}

/**
 * @brief Upgrade a v3 DB with one corrupted legacy stat blob
 *
 * Verify the full upgrade still completes and the corrupted row is stored
 * using the zero-source conversion logic
 */
Return test0015_17(void)
{
	INITTEST;
	const char *corrupted_db_filename = "0015_database_v3_corrupt.db";
	const char *reference_db_filename = "0015_database_v4_reference.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db",corrupted_db_filename));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db",reference_db_filename));

	sqlite3_int64 row_id = 0;
	ASSERT(SUCCESS == db_corrupt_first_row_stat_blob(corrupted_db_filename,&row_id));

	const char *arguments = "--compare --update 0015_database_v4_reference.db 0015_database_v3_corrupt.db";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0015_016.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata(corrupted_db_filename,&db_version));
	ASSERT(db_version == 4);

	CmpctStat stat = {0};
	ASSERT(SUCCESS == db_read_cmpctstat_by_row_id(corrupted_db_filename,row_id,&stat));
	ASSERT(SUCCESS == db_verify_zero_converted_cmpctstat(&stat));

	ASSERT(SUCCESS == delete_path(corrupted_db_filename));
	ASSERT(SUCCESS == delete_path(reference_db_filename));

	m_del(result);
	m_del(pattern);

	RETURN_STATUS;
}

/**
 * @brief Roll back 3->4 migration when SQLite fails during stat conversion
 *
 * Verify the transaction is rolled back and the DB content stays unchanged
 */
Return test0015_18(void)
{
	INITTEST;
	const char *rollback_db_filename = "0015_database_v3_rollback.db";
	const char *reference_db_filename = "0015_database_v4_reference.db";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db",rollback_db_filename));
	ASSERT(SUCCESS == copy_path("tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db",reference_db_filename));

	int files_count = 0;
	ASSERT(SUCCESS == db_read_files_count(rollback_db_filename,&files_count));
	ASSERT(files_count >= 2);

	sqlite3_int64 row_id = 0;
	ASSERT(SUCCESS == db_read_first_row_id(rollback_db_filename,&row_id));

	unsigned char before_blob[512];
	int before_blob_size = 0;
	ASSERT(SUCCESS == db_read_stat_blob_by_row_id(rollback_db_filename,
	                                           row_id,
	                                           before_blob,
	                                           sizeof(before_blob),
	                                           &before_blob_size));

	int v1_rows_before = 0;
	ASSERT(SUCCESS == db_read_files_count_with_blob_size(rollback_db_filename,
	                                                  (int)sizeof(CmpctStat_v1),
	                                                  &v1_rows_before));

	ASSERT(SUCCESS == db_create_abort_on_second_stat_update_trigger(rollback_db_filename));

	const char *arguments = "--compare --update 0015_database_v4_reference.db 0015_database_v3_rollback.db";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));
	const char *filename = "templates/0015_017.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata(rollback_db_filename,&db_version));
	ASSERT(db_version == 3);

	unsigned char after_blob[512];
	int after_blob_size = 0;
	ASSERT(SUCCESS == db_read_stat_blob_by_row_id(rollback_db_filename,
	                                           row_id,
	                                           after_blob,
	                                           sizeof(after_blob),
	                                           &after_blob_size));

	ASSERT(before_blob_size == after_blob_size);

	if(before_blob_size > 0)
	{
		ASSERT(0 == memcmp(before_blob,after_blob,(size_t)before_blob_size));
	}

	int v1_rows_after = 0;
	ASSERT(SUCCESS == db_read_files_count_with_blob_size(rollback_db_filename,
	                                                  (int)sizeof(CmpctStat_v1),
	                                                  &v1_rows_after));
	ASSERT(v1_rows_before == v1_rows_after);

	ASSERT(SUCCESS == delete_path(rollback_db_filename));
	ASSERT(SUCCESS == delete_path(reference_db_filename));

	m_del(result);
	m_del(pattern);

	RETURN_STATUS;
}

/**
 * @brief Warn when metadata declares a DB version newer than supported
 *
 * Verify both testing and non-testing modes keep the future DB unchanged
 */
Return test0015_19(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *db_filename = "0015_database_future_version.db";
	const char *arguments = "--database=0015_database_future_version.db "
	        "tests/fixtures/diffs/diff1";

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == set_db_version_in_metadata(db_filename,CURRENT_DB_VERSION + 1));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata(db_filename,&db_version));
	ASSERT(db_version == CURRENT_DB_VERSION + 1);

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	const char *filename = "templates/0015_018_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == read_db_version_from_metadata(db_filename,&db_version));
	ASSERT(db_version == CURRENT_DB_VERSION + 1);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0015_018_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path(db_filename));

	m_del(result);
	m_del(pattern);

	RETURN_STATUS;
}

/**
 * @brief Exercise DB upgrade and migration regressions across legacy formats
 *
 * This scenario covers primary DB upgrades from legacy versions 0 through 3,
 * comparison-driven upgrades, default DB creation, UTF-8 filename handling,
 * checksum compatibility against a legacy v4 reference DB, resilient handling
 * of corrupted legacy stat blobs, rollback on forced SQLite migration failure,
 * and warning behavior for a DB that reports a future metadata version
 */
Return test0015(void)
{
	INITTEST;

	TEST(test0015_1,"Upgrade a DB from v0 to the current version. Error handling");
	TEST(test0015_2,"Upgrade a DB from v0 to the current version as the primary database");
	TEST(test0015_3,"Upgrade a DB from v0 to the current version with --watch-timestamps");
	TEST(test0015_4,"Verify that the upgraded v0 DB is actually at the current version");
	TEST(test0015_5,"Create default name database");
	TEST(test0015_6,"Attempting an upgrade with a single --compare parameter");
	TEST(test0015_7,"Upgrading from 0 to the last version using the --compare and --update");
	TEST(test0015_8,"Upgrade a DB from v1 to the current version as the primary database");
	TEST(test0015_9,"Verify that the upgraded v1 DB is actually at the current version");
	TEST(test0015_10,"Upgrading from 1 to the last version using the --compare and --update");
	TEST(test0015_11,"Upgrade a DB from v2 to the current version as the primary database");
	TEST(test0015_12,"Upgrading from 2 to the last version using the --compare and --update");
	TEST(test0015_13,"Upgrading from 3 with UTF-8 name to the last version using the --update");
	TEST(test0015_14,"Upgrading from 3 to the last version using the --compare and --update");
	TEST(test0015_15,"Create and compare DBs with UTF-8 names and checksums from legacy DB");
	TEST(test0015_16,"Corrupted v0 stat blob does not break the full upgrade");
	TEST(test0015_17,"Corrupted v3 stat blob does not break the full upgrade");
	TEST(test0015_18,"Forced SQLite failure triggers rollback during 3->4 migration");
	TEST(test0015_19,"Fresh DB with forced future version returns warning and stays unchanged");

	RETURN_STATUS;
}
