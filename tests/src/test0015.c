#include "sute.h"
#include "db_upgrade.h"

#define LEGACY_V3_UTF8_DB "0015_database_v3 это база данных с пробелами и символами UTF-8.db"
#define LEGACY_V4_UTF8_DB "0015_database_v4 это база данных с пробелами и символами UTF-8.db"

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

static Return read_first_row_id(
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

static Return overwrite_stat_blob_by_row_id(
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

static Return read_files_count(
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

static Return read_files_count_with_blob_size(
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

static Return read_db_version_from_metadata(
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

static Return read_stat_blob_by_row_id(
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

static Return read_cmpctstat_by_row_id(
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
		status = read_stat_blob_by_row_id(db_filename,row_id,raw,sizeof(raw),&blob_size);
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

static Return verify_zero_converted_cmpctstat(
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

static Return corrupt_first_row_stat_blob(
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
		status = read_first_row_id(db_filename,&row_id);
	}

	if(SUCCESS == status)
	{
		status = overwrite_stat_blob_by_row_id(db_filename,row_id,corrupt_blob,(int)sizeof(corrupt_blob));
	}

	if(SUCCESS == status && row_id_out != NULL)
	{
		*row_id_out = row_id;
	}

	return(status);
}

static Return create_abort_on_second_stat_update_trigger(
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

	if(db != NULL)
	{
		(void)sqlite3_close(db);
	}

	return(status);
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Verify the run fails without the --update parameter and prints the proper error.
 *
 */
Return test0015_1_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v0.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=./0015_database_v0.db tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_001.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_2_1_upgrade_db(void)
{
	INITTEST;

	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v0.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_002_1.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 0 to the current version as the primary database.
 * Running the test with the --update and --watch-timestamps parameters to ensure
 * the update completes successfully with according details in output
 *
 */
Return test0015_2_2_upgrade_db(void)
{
	INITTEST;

	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v0.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--watch-timestamps --update --database=0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_002_2.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database
 * is actually at the current version
 *
 */
Return test0015_3_upgrade_db(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_003.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v0.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database is actually at the current version
 * Create a database with the default name
 *
 */
Return test0015_4_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *arguments = "tests/examples/diffs/diff1";

	const char *filename = "templates/0015_004.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	RETURN_STATUS;
}

/**
 *
 * Run the program with the --compare parameter to compare databases
 * when one of them has an older version — this should generate an
 * appropriate error message
 *
 */
Return test0015_5_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v0.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_005.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,WARNING));

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare parameter, but this time with
 * the --update option. The database should be upgraded accordingly.
 * Upgrading from 0 to the last version
 *
 */
Return test0015_6_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *arguments = "--compare --update $DBNAME 0015_database_v0.db";

	const char *filename = "templates/0015_006.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v0.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 1 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_7_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v1.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--update --database=0015_database_v1.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_007.txt";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the program again to verify that the database
 * is actually at the current version
 *
 */
Return test0015_8_upgrade_db(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	const char *filename = "templates/0015_008.txt";

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *arguments = "--update --database=./0015_database_v1.db "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	// Clean up test results
	const char *command = "rm \"${TMPDIR}/0015_database_v1.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare and --update parameters.
 * Upgrading from 1 to the last version
 *
 */
Return test0015_9_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v1.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare --update $DBNAME 0015_database_v1.db";

	const char *filename = "templates/0015_009.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v1.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB from version 2 to the current version as the primary database.
 * Running the test with the --update parameter to ensure the update
 * completes successfully
 *
 */
Return test0015_10_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v2.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--update --database=0015_database_v2.db --verbose "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_010.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v2.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * Run the database comparison again using the --compare and --update parameters.
 * Upgrading from 2 to the last version
 *
 */
Return test0015_11_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Get the output of an external program
	const char *command = "cp -a ${ORIGIN_DIR}/tests/templates/0015_database_v2.db ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare --update $DBNAME 0015_database_v2.db";

	const char *filename = "templates/0015_011.txt";  // File name
	const char *template = "%DB_NAME%";

	const char *replacement = getenv("DBNAME");  // Database name

	ASSERT(replacement != NULL);

	ASSERT(SUCCESS == match_app_output(arguments,filename,template,replacement,COMPLETED));

	// Clean up test results
	command = "rm \"${TMPDIR}/${DBNAME}\" && "
	        "rm \"${TMPDIR}/0015_database_v2.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 *
 * Upgrade a DB with UTF-8 name from version 3 to the current version
 * as the primary database using --update.
 *
 */
Return test0015_12_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cp -a \"${ORIGIN_DIR}/tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db\" ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--update --database=\"0015_database_v3 это база данных с пробелами и символами UTF-8.db\" "
	        "tests/examples/diffs/diff1";

	create(char,result);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_012.txt";

	create(char,pattern);

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean up test results
	command = "rm \"${TMPDIR}/0015_database_v3 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Clean to use it iteratively
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Upgrade from version 3 during database comparison using
 * --compare and --update parameters.
 */
Return test0015_13_upgrade_db(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cp -a \"${ORIGIN_DIR}/tests/templates/0015_database_v3 это база данных с пробелами и символами UTF-8.db\" ${TMPDIR}/ && "
	        "cp -a \"${ORIGIN_DIR}/tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db\" ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--compare --update "
	        "\"0015_database_v3 это база данных с пробелами и символами UTF-8.db\" "
	        "\"0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0015_013.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	del(result);
	del(pattern);

	command = "rm \"${TMPDIR}/0015_database_v3 это база данных с пробелами и символами UTF-8.db\" "
	        "\"${TMPDIR}/0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Create a fresh database inside tests/examples/diffs/ with the UTF-8 name
 * "Это новая база данных.db" and ensure the app can read/write it despite
 * spaces and non-ASCII characters.
 * Then compare it against the legacy database
 * "0015_database_v4 это база данных с пробелами и символами UTF-8.db" that was
 * produced by a well-tested older release when upgraded to the version 4.
 * If the files and checksums match, the current checksum calculation is
 * considered compatible with the legacy well-tested algorithm.
 */
Return test0015_14_checksum_compare(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command = "cp -a \"${ORIGIN_DIR}/tests/templates/0015_database_v4 это база данных с пробелами и символами UTF-8.db\" ${TMPDIR}/";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	create(char,pattern);
	create(char,result);
	create(char,chunk);

	const char *arguments = "--database=\"Это новая база данных.db\" "
	        "tests/examples/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == copy(result,chunk));

	arguments = "--compare \"Это новая база данных.db\" "
	        "\"0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == runit(arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == concat_strings(result,chunk));

	const char *filename = "templates/0015_014.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	del(pattern);
	del(result);
	del(chunk);

	// Clean up test results
	command = "rm \"${TMPDIR}/Это новая база данных.db\" \"${TMPDIR}/0015_database_v4 это база данных с пробелами и символами UTF-8.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}

/**
 * Corrupt one v0 row stat blob and verify that upgrade still completes.
 * The corrupted row must end up as converted "zero source" v4 compact stat.
 */
Return test0015_15_corrupt_row_v0_upgrade_continues(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command =
	        "cp -a \"${ORIGIN_DIR}/tests/templates/0015_database_v0.db\" "
	        "\"${TMPDIR}/0015_database_v0_corrupt.db\" && "
	        "cp -a \"${ORIGIN_DIR}/tests/templates/" LEGACY_V4_UTF8_DB "\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	sqlite3_int64 row_id = 0;
	ASSERT(SUCCESS == corrupt_first_row_stat_blob("0015_database_v0_corrupt.db",&row_id));

	create(char,result);
	create(char,pattern);

	const char *arguments = "--compare --update "
	        "0015_database_v4_reference.db 0015_database_v0_corrupt.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0015_015.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata("0015_database_v0_corrupt.db",&db_version));
	ASSERT(db_version == 4);

	CmpctStat stat = {0};
	ASSERT(SUCCESS == read_cmpctstat_by_row_id("0015_database_v0_corrupt.db",row_id,&stat));
	ASSERT(SUCCESS == verify_zero_converted_cmpctstat(&stat));

	command = "rm -f \"${TMPDIR}/0015_database_v0_corrupt.db\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	RETURN_STATUS;
}

/**
 * Corrupt one v3 row stat blob and verify that upgrade still completes.
 * The corrupted row must be stored using zero-source conversion logic.
 */
Return test0015_16_corrupt_row_v3_upgrade_continues(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command =
	        "cp -a \"${ORIGIN_DIR}/tests/templates/" LEGACY_V3_UTF8_DB "\" "
	        "\"${TMPDIR}/0015_database_v3_corrupt.db\" && "
	        "cp -a \"${ORIGIN_DIR}/tests/templates/" LEGACY_V4_UTF8_DB "\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	sqlite3_int64 row_id = 0;
	ASSERT(SUCCESS == corrupt_first_row_stat_blob("0015_database_v3_corrupt.db",&row_id));

	create(char,result);
	create(char,pattern);

	const char *arguments = "--compare --update "
	        "0015_database_v4_reference.db 0015_database_v3_corrupt.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0015_016.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata("0015_database_v3_corrupt.db",&db_version));
	ASSERT(db_version == 4);

	CmpctStat stat = {0};
	ASSERT(SUCCESS == read_cmpctstat_by_row_id("0015_database_v3_corrupt.db",row_id,&stat));
	ASSERT(SUCCESS == verify_zero_converted_cmpctstat(&stat));

	command = "rm -f \"${TMPDIR}/0015_database_v3_corrupt.db\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	RETURN_STATUS;
}

/**
 * Regression test: when a SQLite error occurs during 3->4 migration,
 * the opened transaction must be rolled back.
 */
Return test0015_17_rollback_on_sqlite_failure(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *command =
	        "cp -a \"${ORIGIN_DIR}/tests/templates/" LEGACY_V3_UTF8_DB "\" "
	        "\"${TMPDIR}/0015_database_v3_rollback.db\" && "
	        "cp -a \"${ORIGIN_DIR}/tests/templates/" LEGACY_V4_UTF8_DB "\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	int files_count = 0;
	ASSERT(SUCCESS == read_files_count("0015_database_v3_rollback.db",&files_count));
	ASSERT(files_count >= 2);

	sqlite3_int64 row_id = 0;
	ASSERT(SUCCESS == read_first_row_id("0015_database_v3_rollback.db",&row_id));

	unsigned char before_blob[512];
	int before_blob_size = 0;
	ASSERT(SUCCESS == read_stat_blob_by_row_id("0015_database_v3_rollback.db",
	                                           row_id,
	                                           before_blob,
	                                           sizeof(before_blob),
	                                           &before_blob_size));

	int v1_rows_before = 0;
	ASSERT(SUCCESS == read_files_count_with_blob_size("0015_database_v3_rollback.db",
	                                                  (int)sizeof(CmpctStat_v1),
	                                                  &v1_rows_before));

	ASSERT(SUCCESS == create_abort_on_second_stat_update_trigger("0015_database_v3_rollback.db"));

	create(char,result);
	create(char,pattern);

	const char *arguments = "--compare --update "
	        "0015_database_v4_reference.db 0015_database_v3_rollback.db";

	ASSERT(SUCCESS == runit(arguments,result,NULL,FAILURE,ALLOW_BOTH));
	const char *filename = "templates/0015_017.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	int db_version = 0;
	ASSERT(SUCCESS == read_db_version_from_metadata("0015_database_v3_rollback.db",&db_version));
	ASSERT(db_version == 3);

	unsigned char after_blob[512];
	int after_blob_size = 0;
	ASSERT(SUCCESS == read_stat_blob_by_row_id("0015_database_v3_rollback.db",
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
	ASSERT(SUCCESS == read_files_count_with_blob_size("0015_database_v3_rollback.db",
	                                                  (int)sizeof(CmpctStat_v1),
	                                                  &v1_rows_after));
	ASSERT(v1_rows_before == v1_rows_after);

	command = "rm -f \"${TMPDIR}/0015_database_v3_rollback.db\" "
	        "\"${TMPDIR}/0015_database_v4_reference.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	RETURN_STATUS;
}

/**
 * Generate a fresh database, force future metadata version and verify warning path
 */
Return test0015_18_future_version_warning(void)
{
	INITTEST;

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	const char *db_filename = "0015_database_future_version.db";
	const char *command = "rm -f \"${TMPDIR}/0015_database_future_version.db\"";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=0015_database_future_version.db "
	        "tests/examples/diffs/diff1";

	create(char,result);
	create(char,pattern);

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

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(result);
	del(pattern);

	RETURN_STATUS;
}

/**
 * Testing scenario 15
 *
 * Database upgrade testing:
 * - Upgrade DBs from versions 0, 1, 2, and 3 to the current version as the primary database using --update
 * - Upgrade DBs from versions 0, 1, 2, and 3 during comparison using --compare --update
 * - Launch the program without specifying a database to ensure that a new database is created with the correct version
 * - Compare a current database with an outdated version (v0) without --update and check for the expected error
 * - Validate UTF-8 database names and checksum compatibility against a legacy v4 database
 * - Verify upgrade resilience for corrupted stat blobs in legacy DB v0 and v3
 * - Regression-check rollback behavior on forced SQLite failure during migration
 * - Generate a fresh DB and verify warning behavior for future DB version
 */
Return test0015(void)
{
	INITTEST;

	TEST(test0015_1_upgrade_db,"Upgrade a DB from v0 to the current version. Error handling…");
	TEST(test0015_2_1_upgrade_db,"Upgrade a DB from v0 to the current version as the primary database…");
	TEST(test0015_2_2_upgrade_db,"Upgrade a DB from v0 to the current version with --watch-timestamps…");
	TEST(test0015_3_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_4_upgrade_db,"Create default name database…");
	TEST(test0015_5_upgrade_db,"Attempting an upgrade with a single --compare parameter…");
	TEST(test0015_6_upgrade_db,"Upgrading from 0 to the last version using the --compare and --update…");
	TEST(test0015_7_upgrade_db,"Upgrade a DB from v1 to the current version as the primary database…");
	TEST(test0015_8_upgrade_db,"Verify that the DB is actually at the current version…");
	TEST(test0015_9_upgrade_db,"Upgrading from 1 to the last version using the --compare and --update…");
	TEST(test0015_10_upgrade_db,"Upgrade a DB from v2 to the current version as the primary database…");
	TEST(test0015_11_upgrade_db,"Upgrading from 2 to the last version using the --compare and --update…");
	TEST(test0015_12_upgrade_db,"Upgrading from 3 with UTF-8 name to the last version using the --update…");
	TEST(test0015_13_upgrade_db,"Upgrading from 3 to the last version using the --compare and --update…");
	TEST(test0015_14_checksum_compare,"Create and compare DBs with UTF-8 names and checksums from legacy DB…");
	TEST(test0015_15_corrupt_row_v0_upgrade_continues,"Corrupted v0 stat blob does not break the full upgrade…");
	TEST(test0015_16_corrupt_row_v3_upgrade_continues,"Corrupted v3 stat blob does not break the full upgrade…");
	TEST(test0015_17_rollback_on_sqlite_failure,"Forced SQLite failure triggers rollback during 3->4 migration…");
	TEST(test0015_18_future_version_warning,"Fresh DB with forced future version returns warning and stays unchanged…");

	RETURN_STATUS;
}
