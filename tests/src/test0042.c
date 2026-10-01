#include "sute.h"
#include <errno.h>

/**
 * @brief Create an isolated database with the columns used by file saves
 *
 * @param[in] filename Database filename relative to TMPDIR
 * @param[out] test_config Configuration receiving the writable connection
 * @return SUCCESS when the database is ready, otherwise FAILURE
 */
static Return test0042_open_database(
	const char *filename,
	Config     *test_config)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	ASSERT(SUCCESS == open_db_from_tmpdir(filename,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,&test_config->db));
	ASSERT(SQLITE_OK == sqlite3_exec(test_config->db,
		"CREATE TABLE files (ID INTEGER PRIMARY KEY,offset INTEGER,"
		"relative_path TEXT UNIQUE,sha512 BLOB,stat BLOB,mdContext BLOB);"
		"PRAGMA synchronous=EXTRA;"
		"PRAGMA journal_mode=DELETE;"
		"PRAGMA locking_mode=NORMAL;",NULL,NULL,NULL));
	test_config->sqlite_open_flag = SQLITE_OPEN_READWRITE;

	provide(status);
}

/**
 * @brief Save one empty file through the application file-save path
 *
 * @param[in] filename Relative file path to insert
 * @return SUCCESS when the save succeeds, otherwise FAILURE
 */
static Return test0042_save_file(const char *filename)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	DBrow row = {0};
	File file = {0};
	bool path_known = false;
	m_create(char,relative_path,MEMORY_STRING);

	file.db = &row;
	file.zero_size_file = true;
	ASSERT(SUCCESS == m_copy_string(relative_path,filename));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));

	call(m_del(relative_path));
	provide(status);
}

/**
 * @brief Check one integer result on the current database connection
 *
 * @param[in] sql Query returning one integer column and one row
 * @param expected Expected column value
 * @return SUCCESS when the query returns the expected value, otherwise FAILURE
 */
static Return test0042_expect_integer(
	const char *sql,
	const int  expected)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *statement = NULL;

	ASSERT(SQLITE_OK == sqlite3_prepare_v2(config->db,sql,-1,&statement,NULL));
	ASSERT(SQLITE_ROW == sqlite3_step(statement));
	ASSERT(expected == sqlite3_column_int(statement,0));
	ASSERT(SQLITE_DONE == sqlite3_step(statement));

	if(SQLITE_OK != sqlite3_finalize(statement))
	{
		status = FAILURE;
	}

	provide(status);
}

/**
 * @brief Verify shared timing and visibility of multiple pending file saves
 *
 * A separate connection must see neither file before the deadline and both
 * files after it. Explicit timestamps exercise the boundary without sleeping
 *
 * @return Return status for the transaction timing and persistence checks
 */
static Return test0042_1(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_timed_batch.db";
	long long int started_ns = 0;
	int visible_files = -1;
	struct stat journal_stat = {0};
	m_create(char,journal_path,MEMORY_STRING);

	ASSERT(SUCCESS == test0042_open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SUCCESS == test0042_save_file("first"));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) == 0);
	ASSERT(SUCCESS == test0042_expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	started_ns = test_config.file_transaction_started_ns;
	ASSERT(started_ns > 0);
	ASSERT(SUCCESS == test0042_save_file("second"));
	ASSERT(test_config.file_transaction_started_ns == started_ns);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 0);

	ASSERT(SUCCESS == db_file_transaction_check(started_ns + DB_CHECKPOINT_INTERVAL_NS - 1LL));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 0);
	ASSERT(SUCCESS == db_file_transaction_check(started_ns + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 2);

	ASSERT(SUCCESS == construct_path("0042_timed_batch.db-journal",journal_path));
	errno = 0;
	ASSERT(stat(m_text(journal_path),&journal_stat) == -1);
	ASSERT(errno == ENOENT);
	ASSERT(SUCCESS == db_file_transaction_rollback());

	config = initial_config;

	if(test_config.db != NULL && SQLITE_OK != sqlite3_close(test_config.db))
	{
		status = FAILURE;
	}
	ASSERT(SUCCESS == delete_path(database_filename));
	call(m_del(journal_path));

	RETURN_STATUS;
}

/**
 * @brief Verify final commits, rollback, and preservation of database settings
 *
 * Graceful completion and interruption save the pending files. Technical
 * failure drops only the current batch. Successful writes keep the database
 * modification flag set, including after rollback
 *
 * @return Return status for the finalization scenarios
 */
static Return test0042_2(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_finish_batch.db";
	static const struct {
		const char *filename;
		Return traversal_status;
		bool modified_before;
		int expected_files;
	} scenarios[] = {
		{"completed",SUCCESS,false,1},
		{"interrupted",HALTED,true,2},
		{"failed_after_commit",FAILURE,true,2},
		{"failed_without_prior_changes",FAILURE,false,2}
	};

	ASSERT(SUCCESS == test0042_open_database(database_filename,&test_config));
	config = &test_config;

	for(size_t index = 0; SUCCESS == status && index < sizeof(scenarios) / sizeof(scenarios[0]); index++)
	{
		test_config.db_primary_file_modified = scenarios[index].modified_before;
		ASSERT(SUCCESS == test0042_save_file(scenarios[index].filename));
		ASSERT(test_config.file_transaction_active == true);
		ASSERT(test_config.db_primary_file_modified == true);
		if((scenarios[index].traversal_status & FAILURE) == 0)
		{
			ASSERT(SUCCESS == db_file_transaction_commit());
		}
		ASSERT(SUCCESS == db_file_transaction_rollback());
		ASSERT(test_config.file_transaction_active == false);
		ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
		ASSERT(test_config.db_primary_file_modified == true);
		ASSERT(SUCCESS == test0042_expect_integer("SELECT COUNT(*) FROM files;",scenarios[index].expected_files));
		ASSERT(SUCCESS == test0042_expect_integer(
			"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
			" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
			" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));
	}

	config = initial_config;

	if(test_config.db != NULL && SQLITE_OK != sqlite3_close(test_config.db))
	{
		status = FAILURE;
	}
	ASSERT(SUCCESS == delete_path(database_filename));

	RETURN_STATUS;
}

/**
 * @brief Keep dry-run saves outside transactions for both connection modes
 *
 * Read-only and writable connections preserve their settings and the complete
 * database file. Both cases exercise the ordinary file-save entry point
 *
 * @return Return status for the configuration and file-content checks
 */
static Return test0042_3(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_no_batch.db";
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};
	m_create(char,database_path,MEMORY_STRING);

	ASSERT(SUCCESS == test0042_open_database(database_filename,&test_config));

	if(test_config.db != NULL)
	{
		if(SQLITE_OK != sqlite3_close(test_config.db))
		{
			status = FAILURE;
		}
		test_config.db = NULL;
	}
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_before));
	config = &test_config;

	for(size_t index = 0; SUCCESS == status && index < 2U; index++)
	{
		test_config = (Config){0};
		test_config.dry_run = true;
		test_config.sqlite_open_flag = SQLITE_OPEN_READONLY;

		if(index == 0U)
		{
			test_config.sqlite_open_flag = SQLITE_OPEN_READWRITE;
		}
		ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,test_config.sqlite_open_flag,&test_config.db));

		if(test_config.sqlite_open_flag == SQLITE_OPEN_READWRITE)
		{
			ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
				"PRAGMA synchronous=EXTRA;PRAGMA journal_mode=DELETE;"
				"PRAGMA locking_mode=NORMAL;",NULL,NULL,NULL));
		} else {
			ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
				"PRAGMA synchronous=OFF;PRAGMA locking_mode=EXCLUSIVE;",NULL,NULL,NULL));
		}
		ASSERT(SUCCESS == db_file_transaction_begin());

		ASSERT(SUCCESS == test0042_save_file("simulated"));
		ASSERT(SUCCESS == db_file_transaction_check(cur_time_monotonic_ns() + DB_CHECKPOINT_INTERVAL_NS));
		ASSERT(SUCCESS == db_file_transaction_commit());
		ASSERT(SUCCESS == db_file_transaction_rollback());
		ASSERT(test_config.file_transaction_active == false);
		ASSERT(test_config.db_primary_file_modified == false);
		ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
		ASSERT(SUCCESS == test0042_expect_integer("SELECT COUNT(*) FROM files;",0));

		if(test_config.sqlite_open_flag == SQLITE_OPEN_READWRITE)
		{
			ASSERT(SUCCESS == test0042_expect_integer(
				"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
				" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
				" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));
		} else {
			ASSERT(SUCCESS == test0042_expect_integer(
				"SELECT (SELECT synchronous FROM pragma_synchronous)=0"
				" AND (SELECT locking_mode FROM pragma_locking_mode)='exclusive';",1));
		}

		if(test_config.db != NULL)
		{
			if(SQLITE_OK != sqlite3_close(test_config.db))
			{
				status = FAILURE;
			}
			test_config.db = NULL;
		}
		ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_after));
		ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);
	}

	config = initial_config;
	ASSERT(SUCCESS == delete_path(database_filename));
	call(m_del(database_path));

	RETURN_STATUS;
}

static Return test0042_commit_status = SUCCESS;

/**
 * @brief Capture the expected diagnostic from a blocked transaction commit
 */
static void test0042_capture_commit(void)
{
	test0042_commit_status = db_file_transaction_commit();
}

/**
 * @brief Roll back a batch after a real reader prevents COMMIT
 *
 * The reader holds a shared lock on the committed database. COMMIT must fail
 * with SQLITE_BUSY. Rollback must discard the pending row and release the
 * transaction
 *
 * @return Return status for the failed-commit cleanup checks
 */
static Return test0042_4(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *database_filename = "0042_blocked_commit.db";
	sqlite3 *reader = NULL;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	ASSERT(SUCCESS == test0042_open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	ASSERT(SUCCESS == test0042_save_file("pending"));
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&reader));
	ASSERT(SQLITE_OK == sqlite3_exec(reader,"BEGIN;SELECT COUNT(*) FROM files;",NULL,NULL,NULL));

	test0042_commit_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(test0042_capture_commit,captured_stdout,captured_stderr));
	ASSERT((test0042_commit_status & FAILURE) != 0);
	ASSERT(sqlite3_errcode(test_config.db) == SQLITE_BUSY);
	ASSERT(captured_stdout->length > 0U);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) == 0);
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);

	if(reader != NULL && SQLITE_OK != sqlite3_close(reader))
	{
		status = FAILURE;
	}
	ASSERT(SUCCESS == test0042_expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == test0042_expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	test0042_commit_status = SUCCESS;

	if(test_config.db != NULL && SQLITE_OK != sqlite3_close(test_config.db))
	{
		status = FAILURE;
	}
	ASSERT(SUCCESS == delete_path(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	RETURN_STATUS;
}

/**
 * @brief Run file-transaction timing, completion, and no-write regressions
 *
 * @return Return status for all transaction scenarios
 */
Return test0042(void)
{
	INITTEST;

	TEST(test0042_1,"File batches become visible at their shared deadline");
	TEST(test0042_2,"Completion and interruption commit while errors roll back");
	TEST(test0042_3,"Dry-run and read-only access preserve database contents");
	TEST(test0042_4,"A blocked commit rolls back safely during interruption");

	RETURN_STATUS;
}
