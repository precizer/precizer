#include "sute.h"
#include <errno.h>

/**
 * @brief Create an isolated database with the columns used by file saves
 *
 * @param[in] filename Database filename relative to TMPDIR
 * @param[out] test_config Configuration receiving the writable connection
 * @return SUCCESS when the database is ready, otherwise FAILURE
 */
static Return open_database(
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
 * @brief Check one integer result on the current database connection
 *
 * @param[in] sql Query returning one integer column and one row
 * @param expected Expected column value
 * @return SUCCESS when the query returns the expected value, otherwise FAILURE
 */
static Return expect_integer(
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

	const int finalize_status = sqlite3_finalize(statement);
	ASSERT(SQLITE_OK == finalize_status);

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
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	int visible_files = -1;
	struct stat journal_stat = {0};
	m_create(char,relative_path,MEMORY_STRING);
	m_create(char,journal_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"first"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) == 0);
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	const long long int started_ns = test_config.file_transaction_started_ns;
	ASSERT(started_ns > 0);
	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"second"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
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
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(journal_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Commit a completed batch and preserve the database settings
 *
 * Graceful completion saves the pending file. Rollback after commit does
 * nothing, and successful writes keep the database modification flag set
 *
 * @return Return status for normal completion
 */
static Return test0042_2(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_completed_batch.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(SUCCESS == m_copy_literal(relative_path,"completed"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",1));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Save and commit a file while graceful interruption is active
 *
 * The process status is HALTED during both operations. Their complete return
 * values are checked after restoring the process globals
 *
 * @return Return status for interrupted completion
 */
static Return test0042_3(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const Return initial_global_status = atomic_load(&global_return_status);
	const bool initial_interrupt_flag = atomic_load(&global_interrupt_flag);
	const char *database_filename = "0042_interrupted_batch.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	Return interrupted_save_status = FAILURE;
	Return interrupted_commit_status = FAILURE;
	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"completed"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(test_config.db_primary_file_modified == true);

	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"interrupted"));

	if(SUCCESS == status)
	{
		atomic_store(&global_return_status,HALTED);
		atomic_store(&global_interrupt_flag,true);
		interrupted_save_status = db_save_file_record(relative_path,&file,&path_known,true);
	}
	atomic_store(&global_return_status,initial_global_status);
	atomic_store(&global_interrupt_flag,initial_interrupt_flag);
	ASSERT(interrupted_save_status == (SUCCESS | HALTED));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);

	if(SUCCESS == status)
	{
		atomic_store(&global_return_status,HALTED);
		atomic_store(&global_interrupt_flag,true);
		interrupted_commit_status = db_file_transaction_commit();
	}
	atomic_store(&global_return_status,initial_global_status);
	atomic_store(&global_interrupt_flag,initial_interrupt_flag);
	ASSERT(interrupted_commit_status == (SUCCESS | HALTED));
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Roll back pending writes without losing an earlier committed batch
 *
 * Technical failure drops only the current batch. The two committed files and
 * the database modification flag remain intact
 *
 * @return Return status for rollback after a committed batch
 */
static Return test0042_4(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_rollback_after_commit.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"first"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"second"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));

	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",3));
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Preserve the modification flag when the first changed batch rolls back
 *
 * Existing rows model a database with no changes yet in the current run.
 * Successful writes keep the modification flag set, including after rollback
 *
 * @return Return status for rollback with an initially clear modification flag
 */
static Return test0042_5(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_rollback_initial_batch.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	m_create(char,relative_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
		"INSERT INTO files(relative_path) VALUES('first'),('second');",NULL,NULL,NULL));
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",3));
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Keep writable dry-run saves outside transactions
 *
 * The connection preserves its settings and the complete database file while
 * exercising the ordinary file-save entry point
 *
 * @return Return status for writable dry-run configuration and file contents
 */
static Return test0042_6(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_writable_dry_run.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};
	m_create(char,relative_path,MEMORY_STRING);
	m_create(char,database_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_before));
	config = &test_config;
	test_config.dry_run = true;
	ASSERT(SUCCESS == db_file_transaction_begin());
	ASSERT(SUCCESS == m_copy_literal(relative_path,"simulated"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_check(cur_time_monotonic_ns() + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_after));
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);
	call(delete_path_if_present(database_filename));
	call(m_del(database_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Keep read-only dry-run saves outside transactions
 *
 * The read-only connection preserves its settings and the complete database
 * file while exercising the ordinary file-save entry point
 *
 * @return Return status for read-only dry-run configuration and file contents
 */
static Return test0042_7(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const char *database_filename = "0042_readonly_dry_run.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};
	m_create(char,relative_path,MEMORY_STRING);
	m_create(char,database_path,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	const int setup_close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == setup_close_status);

	if(SQLITE_OK == setup_close_status)
	{
		test_config.db = NULL;
	}
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_before));
	config = &test_config;
	test_config.dry_run = true;
	test_config.sqlite_open_flag = SQLITE_OPEN_READONLY;
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&test_config.db));
	ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
		"PRAGMA synchronous=OFF;PRAGMA locking_mode=EXCLUSIVE;",NULL,NULL,NULL));
	ASSERT(SUCCESS == db_file_transaction_begin());
	ASSERT(SUCCESS == m_copy_literal(relative_path,"simulated"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_check(cur_time_monotonic_ns() + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=0"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='exclusive';",1));

	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_after));
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);
	call(delete_path_if_present(database_filename));
	call(m_del(database_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

static Return test0042_commit_status = SUCCESS;

/**
 * @brief Capture the expected diagnostic from a blocked transaction commit
 */
static void capture_commit(void)
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
static Return test0042_8(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *database_filename = "0042_blocked_commit.db";
	DBrow row = {0};
	File file = {.db = &row,.zero_size_file = true};
	bool path_known = false;
	sqlite3 *reader = NULL;
	m_create(char,relative_path,MEMORY_STRING);
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&reader));
	ASSERT(SQLITE_OK == sqlite3_exec(reader,"BEGIN;SELECT COUNT(*) FROM files;",NULL,NULL,NULL));

	test0042_commit_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(capture_commit,captured_stdout,captured_stderr));
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

	const int reader_close_status = sqlite3_close(reader);
	ASSERT(SQLITE_OK == reader_close_status);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	test0042_commit_status = SUCCESS;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(delete_path_if_present(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(relative_path));

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
	TEST(test0042_2,"Normal completion commits the pending file batch");
	TEST(test0042_3,"Graceful interruption allows file saves and commits");
	TEST(test0042_4,"Rollback preserves earlier committed files");
	TEST(test0042_5,"Rollback preserves the flag set by the first write");
	TEST(test0042_6,"Writable dry-run preserves database contents and settings");
	TEST(test0042_7,"Read-only dry-run preserves database contents and settings");
	TEST(test0042_8,"A blocked commit leaves a batch that can be rolled back");

	RETURN_STATUS;
}
