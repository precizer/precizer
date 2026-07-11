#include "sute.h"

static const char test0038_relative_path[] = "hugetestfile";
static const char test0038_fixture_root[] = "tests/fixtures/huge";
static const int test0038_checkpoint_exit_code = 77;

/**
 * @brief Reset SHA512 checkpoint test hooks in the process environment
 *
 * @return Return status code
 */
static Return test0038_reset_checkpoint_hooks(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	call(set_environment_variable("TESTITALL_TEST_ENV_HASH_CHECKPOINT_AT_RANDOM_BYTE",""));
	call(set_environment_variable("TESTITALL_TEST_ENV_EXIT_AFTER_HASH_CHECKPOINT",""));
	call(set_environment_variable("TESTITALL_TEST_ENV_HASH_CHECKPOINT_EXIT_CODE",""));

	provide(status);
}

/**
 * @brief Enable the random-byte checkpoint hook for one application run
 *
 * @param[in] exit_after_checkpoint Enable abrupt process termination after checkpoint
 *
 * @return Return status code
 */
static Return test0038_enable_checkpoint_hook(const bool exit_after_checkpoint)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_HASH_CHECKPOINT_AT_RANDOM_BYTE","true"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_HASH_CHECKPOINT_EXIT_CODE","77"));

	if(exit_after_checkpoint == true)
	{
		ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_EXIT_AFTER_HASH_CHECKPOINT","true"));

	} else {
		ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_EXIT_AFTER_HASH_CHECKPOINT",""));
	}

	provide(status);
}

/**
 * @brief Remove artifacts used by one checkpoint test case
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 *
 * @return Return status code
 */
static Return test0038_cleanup_case(const char *db_filename)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	call(test0038_reset_checkpoint_hooks());
	call(delete_path_if_present(db_filename));
	call(delete_path_if_present(test0038_fixture_root));

	provide(status);
}

/**
 * @brief Subtest 1
 *
 * @details
 * Forces the application to save a SHA512 checkpoint at a random byte while
 * the same process keeps running. The test then checks that the final database
 * row contains the completed checksum, no resume context, and a digest that
 * matches the Monocypher oracle
 *
 * @return Return status code
 */
static Return test0038_1(void)
{
	INITTEST;

	const char *db_filename = "0038_checkpoint_finalize.db";
	const char *arguments = "--database=0038_checkpoint_finalize.db tests/fixtures/huge";
	m_create(char,huge_file_path,MEMORY_STRING);
	struct stat huge_file_stat = {0};
	int row_count = 0;
	sqlite3_int64 file_id = 0;

	/* Prepare one isolated huge file and enable a checkpoint without process exit */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));
	ASSERT(huge_file_stat.st_size > 0);
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(false));

	/* Let the real application finish after writing the forced checkpoint */
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Verify that the checkpoint was finalized into one clean DB row */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&file_id));
	ASSERT(file_id > 0);
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,test0038_relative_path,m_text(huge_file_path)));
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Always clear hook state and remove per-case artifacts */
	call(test0038_cleanup_case(db_filename));

	m_del(huge_file_path);

	RETURN_STATUS;
}

/**
 * @brief Subtest 2
 *
 * @details
 * Simulates a sudden process exit immediately after a durable SHA512
 * checkpoint. The next application run must resume from the saved offset and
 * context, finish the hash, clear the resume state, and store a checksum that
 * matches the Monocypher oracle
 *
 * @return Return status code
 */
static Return test0038_2(void)
{
	INITTEST;

	const char *db_filename = "0038_checkpoint_crash_resume.db";
	const char *first_arguments = "--database=0038_checkpoint_crash_resume.db tests/fixtures/huge";
	const char *second_arguments = "--update --database=0038_checkpoint_crash_resume.db tests/fixtures/huge";
	m_create(char,huge_file_path,MEMORY_STRING);
	struct stat huge_file_stat = {0};
	int row_count = 0;
	sqlite3_int64 checkpoint_offset = 0;
	int checkpoint_md_context_bytes = 0;

	/* Prepare the fixture and make the application exit after saving a checkpoint */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(true));

	/* The controlled exit code proves that the crash hook, not a test timeout, stopped the process */
	ASSERT(SUCCESS == runit_background(
		first_arguments,
		NULL,
		NULL,
		test0038_checkpoint_exit_code,
		ALLOW_BOTH | STDERR_ALLOW,
		0U,
		5000U,
		0,
		0U));

	/* Inspect the durable partial SHA512 state left by the interrupted run */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == read_resume_state_from_db(db_filename,test0038_relative_path,&checkpoint_offset,&checkpoint_md_context_bytes));
	ASSERT(checkpoint_offset > 0);
	ASSERT(checkpoint_offset < (sqlite3_int64)huge_file_stat.st_size);
	ASSERT(checkpoint_md_context_bytes > 0);

	/* Restart without crash hooks and finish from the saved checkpoint */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == runit(second_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Verify that resume ended in a clean final checksum state */
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,test0038_relative_path,m_text(huge_file_path)));
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Always clear hook state and remove per-case artifacts */
	call(test0038_cleanup_case(db_filename));

	m_del(huge_file_path);

	RETURN_STATUS;
}

/**
 * @brief Subtest 3
 *
 * @details
 * Crashes once during initial hashing and once during resumed hashing. Both
 * partial checkpoints must update the same database row, the second offset
 * must move forward, and the final run must finish that same row with a valid
 * SHA512 digest
 *
 * @return Return status code
 */
static Return test0038_3(void)
{
	INITTEST;

	const char *db_filename = "0038_checkpoint_update_same_row.db";
	const char *first_arguments = "--database=0038_checkpoint_update_same_row.db tests/fixtures/huge";
	const char *update_arguments = "--update --database=0038_checkpoint_update_same_row.db tests/fixtures/huge";
	m_create(char,huge_file_path,MEMORY_STRING);
	struct stat huge_file_stat = {0};
	int row_count = 0;
	sqlite3_int64 first_id = 0;
	sqlite3_int64 second_id = 0;
	sqlite3_int64 final_id = 0;
	sqlite3_int64 first_offset = 0;
	sqlite3_int64 second_offset = 0;
	int first_md_context_bytes = 0;
	int second_md_context_bytes = 0;

	/* Create the first durable partial checkpoint in a fresh database row */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(true));
	ASSERT(SUCCESS == runit_background(
		first_arguments,
		NULL,
		NULL,
		test0038_checkpoint_exit_code,
		ALLOW_BOTH | STDERR_ALLOW,
		0U,
		5000U,
		0,
		0U));

	/* Record the row identity and first saved offset after the first crash */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&first_id));
	ASSERT(SUCCESS == read_resume_state_from_db(db_filename,test0038_relative_path,&first_offset,&first_md_context_bytes));
	ASSERT(first_id > 0);
	ASSERT(first_offset > 0);
	ASSERT(first_offset < (sqlite3_int64)huge_file_stat.st_size);
	ASSERT(first_offset < ((sqlite3_int64)huge_file_stat.st_size - 1));
	ASSERT(first_md_context_bytes > 0);

	/* Resume once, checkpoint again, and crash after updating the same row */
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(true));
	ASSERT(SUCCESS == runit_background(
		update_arguments,
		NULL,
		NULL,
		test0038_checkpoint_exit_code,
		ALLOW_BOTH | STDERR_ALLOW,
		0U,
		5000U,
		0,
		0U));

	/* The second checkpoint must keep row identity and advance the offset */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&second_id));
	ASSERT(SUCCESS == read_resume_state_from_db(db_filename,test0038_relative_path,&second_offset,&second_md_context_bytes));
	ASSERT(second_id == first_id);
	ASSERT(second_offset > first_offset);
	ASSERT(second_offset < (sqlite3_int64)huge_file_stat.st_size);
	ASSERT(second_md_context_bytes > 0);

	/* Finish from the second checkpoint and verify that the same row becomes final */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == runit(update_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&final_id));
	ASSERT(final_id == first_id);
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,test0038_relative_path,m_text(huge_file_path)));
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Always clear hook state and remove per-case artifacts */
	call(test0038_cleanup_case(db_filename));

	m_del(huge_file_path);

	RETURN_STATUS;
}

/**
 * @brief Subtest 4
 *
 * @details
 * Verifies that a completed checksum-locked row cannot be replaced by a
 * temporary checkpoint state. Even when `--rehash-locked` reads the file again,
 * the database must keep the trusted final checksum and must not store a
 * partial offset or SHA512 context
 *
 * @return Return status code
 */
static Return test0038_4(void)
{
	INITTEST;

	const char *db_filename = "0038_checkpoint_lock_checksum.db";
	const char *create_arguments = "--database=0038_checkpoint_lock_checksum.db "
	        "--lock-checksum=\"^hugetestfile$\" tests/fixtures/huge";
	const char *update_arguments = "--update --rehash-locked --database=0038_checkpoint_lock_checksum.db "
	        "--lock-checksum=\"^hugetestfile$\" tests/fixtures/huge";
	m_create(char,huge_file_path,MEMORY_STRING);
	struct stat huge_file_stat = {0};
	sqlite3_int64 id_before = 0;
	sqlite3_int64 id_after = 0;
	sqlite3_int64 offset_before = -1;
	sqlite3_int64 offset_after = -1;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/* Create the sealed checksum-locked baseline row */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));
	ASSERT(huge_file_stat.st_size > 0);
	ASSERT(SUCCESS == runit(create_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Capture the trusted final state before the forced checkpoint hook runs */
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&id_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,test0038_relative_path,&offset_before,sha512_before));
	ASSERT(offset_before == 0);
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,test0038_relative_path,m_text(huge_file_path)));

	/* Rehash the locked file while the checkpoint hook tries to save partial state */
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(false));
	ASSERT(SUCCESS == runit(update_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* The locked row must remain final and byte-for-byte unchanged */
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&id_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,test0038_relative_path,&offset_after,sha512_after));
	ASSERT(id_after == id_before);
	ASSERT(offset_after == 0);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Always clear hook state and remove per-case artifacts */
	call(test0038_cleanup_case(db_filename));

	m_del(huge_file_path);

	RETURN_STATUS;
}

/**
 * @brief Subtest 5
 *
 * @details
 * Checks that `--dry-run=with-checksums` may calculate SHA512 data but still
 * leaves persistent database state untouched. A forced checkpoint during the
 * dry run must not change the row ID, final offset, final digest, or resume
 * context stored by the earlier real run
 *
 * @return Return status code
 */
static Return test0038_5(void)
{
	INITTEST;

	const char *db_filename = "0038_checkpoint_dry_run.db";
	const char *create_arguments = "--database=0038_checkpoint_dry_run.db tests/fixtures/huge";
	const char *dry_run_arguments = "--dry-run=with-checksums --update --database=0038_checkpoint_dry_run.db tests/fixtures/huge";
	m_create(char,huge_file_path,MEMORY_STRING);
	struct stat huge_file_stat = {0};
	int row_count = 0;
	sqlite3_int64 id_before = 0;
	sqlite3_int64 id_after = 0;
	sqlite3_int64 offset_before = -1;
	sqlite3_int64 offset_after = -1;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/* Create a real final row that the later dry run must not change */
	ASSERT(SUCCESS == test0038_reset_checkpoint_hooks());
	ASSERT(SUCCESS == prepare_huge_fixture(huge_file_path,&huge_file_stat));
	ASSERT(huge_file_stat.st_size > 0);
	ASSERT(SUCCESS == runit(create_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Save the baseline DB identity and final checksum state */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&id_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,test0038_relative_path,&offset_before,sha512_before));
	ASSERT(offset_before == 0);
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Force a checkpoint opportunity during checksum-enabled dry-run mode */
	ASSERT(SUCCESS == test0038_enable_checkpoint_hook(false));
	ASSERT(SUCCESS == runit(dry_run_arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	/* Dry-run must leave the previously saved final DB state untouched */
	ASSERT(SUCCESS == db_read_files_count(db_filename,&row_count));
	ASSERT(row_count == 1);
	ASSERT(SUCCESS == db_read_file_id(db_filename,test0038_relative_path,&id_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,test0038_relative_path,&offset_after,sha512_after));
	ASSERT(id_after == id_before);
	ASSERT(offset_after == 0);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,test0038_relative_path));

	/* Always clear hook state and remove per-case artifacts */
	call(test0038_cleanup_case(db_filename));

	m_del(huge_file_path);

	RETURN_STATUS;
}

/**
 * @brief Run SHA512 random-byte checkpoint and resume system subtests
 *
 * @details
 * These subtests exercise the real application binary instead of calling
 * internal hashing functions directly. They verify that periodic SHA512
 * checkpoints are written only when persistent state is allowed, survive a
 * controlled process exit, resume into the same database row, and disappear
 * after the final checksum is saved. The suite is slow because it hashes the
 * huge-file fixture several times and launches separate application processes
 *
 * @return Return status code
 */
Return test0038(void)
{
	INITTEST;

	SLOWTEST;

	TEST(test0038_1,"Random-byte checkpoint leaves only final state after one pass");
	TEST(test0038_2,"Abrupt exit after checkpoint persists resume state across restart");
	TEST(test0038_3,"Resume checkpoint updates the same row before finalization");
	TEST(test0038_4,"Checksum-locked sealed row rejects partial checkpoint state");
	TEST(test0038_5,"Dry-run with checksums does not persist checkpoint state");

	RETURN_STATUS;
}
