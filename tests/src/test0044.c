#include "sute.h"

static Return cleanup_status = SUCCESS;

/**
 * @brief Prepare two missing file records under an existing fixture root
 *
 * Each scenario uses its own real SQLite file. Only the columns required by
 * missing-file cleanup are created. DELETE journaling and NORMAL locking allow
 * a separate reader to block commit without preventing the writer from starting
 *
 * @param[in] filename Database filename relative to TMPDIR
 * @param[out] test_config Configuration receiving the writable connection
 * @return SUCCESS when both records and the root are stored, otherwise FAILURE
 */
static Return open_database(
	const char *filename,
	Config     *test_config)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3_stmt *statement = NULL;
	m_create(char,root_path,MEMORY_STRING);

	test_config->db_file_name = m_init(char,MEMORY_STRING);
	test_config->update = true;
	test_config->db_primary_file_exists = true;
	test_config->sqlite_open_flag = SQLITE_OPEN_READWRITE;

	/* Use the existing copied fixture without changing any of its files */
	ASSERT(SUCCESS == construct_path("tests/fixtures/diffs/diff1",root_path));
	ASSERT(SUCCESS == m_copy_string(&test_config->db_file_name,filename));
	ASSERT(SUCCESS == open_db_from_tmpdir(filename,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,&test_config->db));
	ASSERT(SQLITE_OK == sqlite3_exec(test_config->db,
		"PRAGMA journal_mode=DELETE;"
		"PRAGMA locking_mode=NORMAL;"
		"CREATE TABLE paths(prefix TEXT NOT NULL);"
		"CREATE TABLE files(ID INTEGER PRIMARY KEY,relative_path TEXT NOT NULL);"
		"INSERT INTO files VALUES(1,'0044_missing_first'),(2,'0044_missing_second');",
		NULL,NULL,NULL));

	/* Bind the absolute root so its spelling does not need SQL escaping */
	ASSERT(SQLITE_OK == sqlite3_prepare_v2(test_config->db,
		"INSERT INTO paths(prefix) VALUES(?1);",-1,&statement,NULL));
	ASSERT(SQLITE_OK == sqlite3_bind_text(statement,1,m_text(root_path),-1,SQLITE_TRANSIENT));
	ASSERT(SQLITE_DONE == sqlite3_step(statement));

	/* Release preparation resources even when an assertion failed */
	const int finalize_status = sqlite3_finalize(statement);
	ASSERT(SQLITE_OK == finalize_status);
	call(m_del(root_path));

	provide(status);
}

/**
 * @brief Capture cleanup diagnostics without exposing interruption to the runner
 *
 * The cleanup result is kept separately from capture success. Interruption
 * globals are restored before the capture helper resumes reading its buffers
 */
static void capture_cleanup(void)
{
	const Return initial_global_status = atomic_load(&global_return_status);
	const bool initial_interrupt_flag = atomic_load(&global_interrupt_flag);

	cleanup_status = db_delete_missing_metadata();

	atomic_store(&global_return_status,initial_global_status);
	atomic_store(&global_interrupt_flag,initial_interrupt_flag);
}

/**
 * @brief Roll back the first deletion when the second deletion fails
 *
 * The trigger fails only after one of the two rows has already been removed,
 * regardless of their iteration order. Both original paths must remain visible
 * to another connection after cleanup returns its technical failure
 *
 * @return Return status for partial-deletion rollback
 */
static Return test0044_1(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *database_filename = "0044_failed_delete.db";
	const char *expected_paths[] = {"0044_missing_first","0044_missing_second"};
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	/* Fail the second deletion without automatically rolling back the transaction */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
		"CREATE TRIGGER abort_second_delete BEFORE DELETE ON files "
		"WHEN (SELECT COUNT(*) FROM files)=1 "
		"BEGIN SELECT RAISE(ABORT,'forced deletion failure'); END;",
		NULL,NULL,NULL));

	/* Require the intended failure and restoration of both committed records */
	cleanup_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(capture_cleanup,captured_stdout,captured_stderr));
	ASSERT((cleanup_status & FAILURE) != 0);
	ASSERT(NULL != strstr(m_text(captured_stdout),"forced deletion failure"));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == db_paths_match(database_filename,expected_paths,2));

	/* Restore shared state and close the connection after any assertion result */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	cleanup_status = SUCCESS;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(m_del(&test_config.db_file_name));
	call(delete_path_if_present(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	RETURN_STATUS;
}

/**
 * @brief Roll back cleanup when a reader prevents its commit
 *
 * A separate read transaction permits DELETE statements but blocks COMMIT in
 * DELETE journal mode. Cleanup must return failure and roll back its deletions
 * while the reader is still open, leaving both original paths committed
 *
 * @return Return status for recovery from a blocked cleanup commit
 */
static Return test0044_2(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *database_filename = "0044_blocked_commit.db";
	const char *expected_paths[] = {"0044_missing_first","0044_missing_second"};
	sqlite3 *reader = NULL;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	/* Keep a real shared lock for the entire cleanup and its rollback */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&reader));
	ASSERT(SQLITE_OK == sqlite3_exec(reader,"BEGIN;SELECT COUNT(*) FROM files;",NULL,NULL,NULL));

	/* The error must come from COMMIT, and no deletion may remain pending */
	cleanup_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(capture_cleanup,captured_stdout,captured_stderr));
	ASSERT((cleanup_status & FAILURE) != 0);
	ASSERT(NULL != strstr(m_text(captured_stdout),"Failed to commit missing file cleanup transaction"));
	ASSERT(captured_stderr->length == 0U);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == db_paths_match(database_filename,expected_paths,2));

	/* Close both connections and restore shared state even after a failed check */
	const int reader_close_status = sqlite3_close(reader);
	ASSERT(SQLITE_OK == reader_close_status);
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	cleanup_status = SUCCESS;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(m_del(&test_config.db_file_name));
	call(delete_path_if_present(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	RETURN_STATUS;
}

/**
 * @brief Request graceful interruption after a file record is deleted
 *
 * @param[in,out] context Number of observed file deletions
 * @param operation SQLite operation reported by the update hook
 * @param database_name Name of the database containing the changed table
 * @param table_name Name of the changed table
 * @param row_id ID of the changed row
 */
static void interrupt_after_delete(
	void          *context,
	int           operation,
	const char    *database_name,
	const char    *table_name,
	sqlite3_int64 row_id)
{
	(void)operation;
	(void)database_name;
	(void)table_name;
	(void)row_id;

	int *deleted_count = context;
	++(*deleted_count);
	atomic_store(&global_interrupt_flag,true);
	atomic_store(&global_return_status,HALTED);
}

/**
 * @brief Commit completed deletions when interruption arrives during cleanup
 *
 * The update hook raises interruption only after the first real DELETE.
 * Cleanup must stop before another deletion and commit the completed work.
 * This isolates the interruption boundary without signals, sleeps, or timers
 *
 * @return Return status for graceful cleanup completion
 */
static Return test0044_3(void)
{
	INITTEST;
	Config *initial_config = config;
	Config test_config = {0};
	const LOGMODES initial_logger_mode = atomic_load_explicit(&rational_logger_mode,memory_order_relaxed);
	const char *database_filename = "0044_interrupted_cleanup.db";
	int deleted_count = 0;
	int visible_files = -1;
	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);

	/* Install the interrupt hook only after both baseline records are committed */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	IF(SUCCESS == status)
	{
		(void)sqlite3_update_hook(test_config.db,interrupt_after_delete,&deleted_count);
	}

	/* Capture restores interruption globals before any later assertions run */
	cleanup_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(capture_cleanup,captured_stdout,captured_stderr));
	ASSERT(cleanup_status == (SUCCESS | HALTED));
	ASSERT(deleted_count == 1);
	ASSERT(captured_stderr->length == 0U);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 1);

	/* Remove the hook before its stack-based context leaves scope */
	IF(test_config.db != NULL)
	{
		(void)sqlite3_update_hook(test_config.db,NULL,NULL);
	}

	/* Restore shared state and release all resources after any assertion result */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	cleanup_status = SUCCESS;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);
	call(m_del(&test_config.db_file_name));
	call(delete_path_if_present(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	RETURN_STATUS;
}

/**
 * @brief Run missing-file cleanup transaction regressions
 *
 * Each scenario calls cleanup directly against two missing file records.
 * Real SQLite failures check atomic rollback, and a deletion hook checks that
 * graceful interruption commits completed deletions without processing more rows
 *
 * @return Return status for all cleanup transaction scenarios
 */
Return test0044(void)
{
	INITTEST;

	TEST(test0044_1,"A failed deletion restores earlier deletions in the cleanup batch");
	TEST(test0044_2,"A blocked cleanup commit rolls back all deletions");
	TEST(test0044_3,"Graceful interruption commits completed cleanup deletions");

	RETURN_STATUS;
}
