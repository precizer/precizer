#include "sute.h"
#include <errno.h>

/**
 * @brief Create an isolated database with the columns used by file saves
 *
 * Each scenario gets its own real SQLite file and a minimal files table.
 * The connection uses EXTRA synchronization, DELETE journaling, and NORMAL
 * locking so transaction behavior can be tested without application startup
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

	/* Create a real database file for this scenario */
	ASSERT(SUCCESS == open_db_from_tmpdir(filename,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,&test_config->db));

	/* Create the file-record schema and establish the writable connection settings */
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

	/* Read one integer and require the query to return exactly one row */
	ASSERT(SQLITE_OK == sqlite3_prepare_v2(config->db,sql,-1,&statement,NULL));
	ASSERT(SQLITE_ROW == sqlite3_step(statement));
	ASSERT(expected == sqlite3_column_int(statement,0));
	ASSERT(SQLITE_DONE == sqlite3_step(statement));

	/* Release the statement even if preparation or a result check failed */
	const int finalize_status = sqlite3_finalize(statement);
	ASSERT(SQLITE_OK == finalize_status);

	provide(status);
}

/**
 * @brief Verify shared timing and visibility of multiple pending file saves
 *
 * Two file records must share one transaction and its original start time.
 * A separate connection must see neither record before the deadline and both
 * records after commit. Explicit timestamps exercise one nanosecond before
 * the deadline and the deadline itself without sleeping. A successful commit
 * must also remove the rollback journal in DELETE mode
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

	/* Save the first file record and verify that it starts a transaction */
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

	/* Save another record in the same batch without moving its start time */
	const long long int started_ns = test_config.file_transaction_started_ns;
	ASSERT(started_ns > 0);
	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"second"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_started_ns == started_ns);

	/* A separate connection must not see either pending record */
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 0);

	/* Supply a time just before the deadline; the batch must remain pending */
	ASSERT(SUCCESS == db_file_transaction_check(started_ns + DB_CHECKPOINT_INTERVAL_NS - 1LL));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 0);

	/* At the deadline, both records must become visible to another connection */
	ASSERT(SUCCESS == db_file_transaction_check(started_ns + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == db_read_files_count(database_filename,&visible_files));
	ASSERT(visible_files == 2);

	/* DELETE mode must remove the journal when the transaction commits */
	ASSERT(SUCCESS == construct_path("0042_timed_batch.db-journal",journal_path));
	errno = 0;
	ASSERT(stat(m_text(journal_path),&journal_stat) == -1);
	ASSERT(errno == ENOENT);

	/* A cleanup rollback after commit must succeed without an active transaction */
	ASSERT(SUCCESS == db_file_transaction_rollback());

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(journal_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Commit a completed batch and preserve the database settings
 *
 * The test directly calls commit and then rollback to model normal traversal
 * cleanup. The record must remain saved, SQLite must leave the transaction,
 * and connection settings must stay unchanged. The successful write keeps the
 * database modification flag set
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

	/* Begin with an unmodified database and save one pending record */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(SUCCESS == m_copy_literal(relative_path,"completed"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);

	/* Model normal completion: commit the batch, then run rollback cleanup */
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());

	/* The committed record must survive cleanup and the settings must be preserved */
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",1));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Save and commit a file while graceful interruption is active
 *
 * A previously committed record establishes the starting state. A second
 * record is saved and committed with HALTED and the interrupt flag set.
 * Each operation must return SUCCESS | HALTED and both records must remain.
 * Results are kept separately from the test status, and process globals are
 * restored before checking those results or performing other test operations.
 *
 * The flags are set directly to isolate behavior after interruption. Signal
 * delivery and completion of the filesystem traversal are outside this test
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

	/* Commit a baseline record before simulating interruption */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"completed"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(test_config.db_primary_file_modified == true);

	/* Prepare another independent file record for the interrupted save */
	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"interrupted"));

	/* Set the state produced by graceful interruption and save directly.
	   Keep the returned HALTED flag separate from the test status */
	if(SUCCESS == status)
	{
		atomic_store(&global_return_status,HALTED);
		atomic_store(&global_interrupt_flag,true);
		interrupted_save_status = db_save_file_record(relative_path,&file,&path_known,true);
	}

	/* Restore globals before checking the saved result so simulated interruption
	   cannot affect later helper returns or escape into the test runner */
	atomic_store(&global_return_status,initial_global_status);
	atomic_store(&global_interrupt_flag,initial_interrupt_flag);
	ASSERT(interrupted_save_status == (SUCCESS | HALTED));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);

	/* Commit must also run successfully while the same interruption flags are set */
	if(SUCCESS == status)
	{
		atomic_store(&global_return_status,HALTED);
		atomic_store(&global_interrupt_flag,true);
		interrupted_commit_status = db_file_transaction_commit();
	}

	/* Restore both globals even if the commit failed, then check its full status */
	atomic_store(&global_return_status,initial_global_status);
	atomic_store(&global_interrupt_flag,initial_interrupt_flag);
	ASSERT(interrupted_commit_status == (SUCCESS | HALTED));

	/* Cleanup must preserve both committed records and the connection settings */
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Roll back pending writes without losing an earlier committed batch
 *
 * Two records are committed before a third record starts another transaction.
 * An explicit rollback must remove only the third record and keep the database
 * modification flag set. This models cleanup after a technical failure by
 * calling rollback directly; no traversal failure is injected
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

	/* Save and commit two records to establish an earlier completed batch */
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

	/* Start another batch; the writer can see its third, uncommitted record */
	row = (DBrow){0};
	file = (File){.db = &row,.zero_size_file = true};
	path_known = false;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",3));

	/* Explicitly roll back the current batch and keep the two committed records */
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Preserve the modification flag when the first changed batch rolls back
 *
 * Setup inserts existing rows directly through SQLite, leaving the application's
 * modification flag clear. Saving a new file record must set that flag.
 * Rollback removes the new record but preserves the existing rows and the flag:
 * the flag records that a write succeeded during the run, including writes
 * whose transaction was later rolled back
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

	/* Seed existing records directly so setup does not set the application flag */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
		"INSERT INTO files(relative_path) VALUES('first'),('second');",NULL,NULL,NULL));
	ASSERT(test_config.db_primary_file_modified == false);

	/* The first application save must set the modification flag before commit */
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",3));

	/* Rollback removes the pending record but leaves the flag set by that write */
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",2));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Keep writable dry-run saves outside transactions
 *
 * A writable connection makes the application's dry-run guard responsible for
 * preventing writes. The test calls begin, file save, deadline check, commit,
 * and rollback, then verifies an empty table, no active transaction, and the
 * original connection settings. Whole-file SHA512 values before the sequence
 * and after closing the connection must match
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

	/* Prepare a writable database and hash its complete initial contents */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_before));
	config = &test_config;
	/* Exercise the transaction entry points and file save with dry-run enabled */
	test_config.dry_run = true;
	ASSERT(SUCCESS == db_file_transaction_begin());
	ASSERT(SUCCESS == m_copy_literal(relative_path,"simulated"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_check(cur_time_monotonic_ns() + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());

	/* No record or active transaction may remain, and settings must be unchanged */
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT journal_mode FROM pragma_journal_mode)='delete'"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* After closing SQLite, compare the entire database file with the original */
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_after));
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(database_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Keep read-only dry-run saves outside transactions
 *
 * The prepared database is reopened with the application's read-only
 * synchronization and locking settings. The dry-run calls must succeed while
 * leaving those settings intact, adding no rows, and leaving no transaction
 * active. Whole-file SHA512 values before the sequence and after closing the
 * connection must match
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

	/* Create the database through a writable connection before reopening it read-only */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	const int setup_close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == setup_close_status);

	/* Clear a closed handle to prevent a second close. If closing failed, keep
	   the handle available for the unconditional cleanup below */
	if(SQLITE_OK == setup_close_status)
	{
		test_config.db = NULL;
	}

	/* Hash the prepared file before any dry-run operation */
	ASSERT(SUCCESS == construct_path(database_filename,database_path));
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_before));
	config = &test_config;
	/* Reopen read-only so the dry-run calls must work without write access */
	test_config.dry_run = true;
	test_config.sqlite_open_flag = SQLITE_OPEN_READONLY;
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&test_config.db));

	/* Use the read-only settings from db_init(). Dry-run must preserve them
	   instead of applying the writable connection settings */
	ASSERT(SQLITE_OK == sqlite3_exec(test_config.db,
		"PRAGMA synchronous=OFF;PRAGMA locking_mode=EXCLUSIVE;",NULL,NULL,NULL));

	/* Exercise the same transaction and save calls as the writable dry-run case */
	ASSERT(SUCCESS == db_file_transaction_begin());
	ASSERT(SUCCESS == m_copy_literal(relative_path,"simulated"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));
	ASSERT(SUCCESS == db_file_transaction_check(cur_time_monotonic_ns() + DB_CHECKPOINT_INTERVAL_NS));
	ASSERT(SUCCESS == db_file_transaction_commit());
	ASSERT(SUCCESS == db_file_transaction_rollback());

	/* Require no active transaction, no inserted row, and unchanged settings */
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == false);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=0"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='exclusive';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* After closing SQLite, verify that the complete file contents are unchanged */
	ASSERT(SUCCESS == compute_file_sha512_monocypher(m_text(database_path),checksum_after));
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(database_path));
	call(m_del(relative_path));

	RETURN_STATUS;
}

static Return commit_status = SUCCESS;

/**
 * @brief Capture the expected diagnostic from a blocked transaction commit
 *
 * The capture callback has no return value, so the commit result is saved
 * separately to distinguish the expected commit failure from a capture failure
 */
static void capture_commit(void)
{
	commit_status = db_file_transaction_commit();
}

/**
 * @brief Roll back a batch after a real reader prevents COMMIT
 *
 * A separate connection keeps a read transaction open while the writer has
 * a pending record. Its shared lock must make COMMIT fail with SQLITE_BUSY,
 * leaving the writer's transaction active. An explicit rollback must then
 * discard the pending record and end that transaction while the reader is
 * still open. The write keeps the database modification flag set
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

	/* Create a writer transaction containing one pending record */
	ASSERT(SUCCESS == open_database(database_filename,&test_config));
	config = &test_config;
	rational_logger_mode = REGULAR;
	ASSERT(SUCCESS == m_copy_literal(relative_path,"pending"));
	ASSERT(SUCCESS == db_save_file_record(relative_path,&file,&path_known,true));

	/* BEGIN keeps the reader transaction open after SELECT acquires its shared
	   lock. Keeping this reader open prevents the writer from committing */
	ASSERT(SUCCESS == open_db_from_tmpdir(database_filename,SQLITE_OPEN_READONLY,&reader));
	ASSERT(SQLITE_OK == sqlite3_exec(reader,"BEGIN;SELECT COUNT(*) FROM files;",NULL,NULL,NULL));

	/* Capture the expected error while keeping the commit result separate */
	commit_status = SUCCESS;
	ASSERT(SUCCESS == function_capture(capture_commit,captured_stdout,captured_stderr));
	ASSERT((commit_status & FAILURE) != 0);
	ASSERT(sqlite3_errcode(test_config.db) == SQLITE_BUSY);
	ASSERT(captured_stdout->length > 0U);
	ASSERT(captured_stderr->length == 0U);

	/* A blocked commit must leave the writer transaction available for rollback */
	ASSERT(test_config.file_transaction_active == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) == 0);

	/* Roll back while the reader is still open; the successful write keeps
	   the modification flag set even though its row is discarded */
	ASSERT(SUCCESS == db_file_transaction_rollback());
	ASSERT(test_config.file_transaction_active == false);
	ASSERT(test_config.db_primary_file_modified == true);
	ASSERT(sqlite3_get_autocommit(test_config.db) != 0);

	/* Close the reader even after a failed check, then verify the empty table */
	const int reader_close_status = sqlite3_close(reader);
	ASSERT(SQLITE_OK == reader_close_status);
	ASSERT(SUCCESS == expect_integer("SELECT COUNT(*) FROM files;",0));
	ASSERT(SUCCESS == expect_integer(
		"SELECT (SELECT synchronous FROM pragma_synchronous)=3"
		" AND (SELECT locking_mode FROM pragma_locking_mode)='normal';",1));

	/* Restore shared state and close the test connection even after a failed check */
	config = initial_config;
	rational_logger_mode = initial_logger_mode;
	commit_status = SUCCESS;
	const int close_status = sqlite3_close(test_config.db);
	ASSERT(SQLITE_OK == close_status);

	/* Remove the temporary database and release the scenario buffers */
	call(delete_path_if_present(database_filename));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Run file-transaction timing, completion, and no-write regressions
 *
 * The scenarios call file-save and transaction functions against independent
 * SQLite files. File records represent empty files, so saving them requires
 * no traversal or input-file reads. Pending records must become
 * visible to another connection only after commit; rollback must discard the
 * pending batch while preserving previously committed records.
 *
 * Explicit timestamps exercise the commit deadline without waiting. Directly
 * assigned interruption flags check that saving and committing still work with
 * HALTED. Dry-run cases compare database-file hashes and connection settings,
 * and a real reader lock exercises recovery from a blocked commit.
 *
 * These checks cover the called functions. Signal delivery and the traversal's
 * decision to call commit or rollback are outside their scope
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
