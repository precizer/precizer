#include "sute.h"

/**
 * @brief Corrupt stored SHA512 bytes for one locked file row in DB
 *
 * @param[in] db_filename Database filename relative to TMPDIR
 * @param[in] relative_path Relative path key in files table
 *
 * @return Return status code:
 *         - SUCCESS: SHA512 value was updated for at least one row
 *         - FAILURE: Validation, DB access, bind, step, or change check failed
 */
static Return db_tamper_locked_checksum(
	const char *db_filename,
	const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "UPDATE files SET sha512 = (substr(sha512,1,2) || X'BEEF' || substr(sha512,5)) " "WHERE relative_path = ?1;";

	m_create(char,db_path,MEMORY_STRING);

	if(SUCCESS == status && (db_filename == NULL || relative_path == NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(m_text(db_path),&db,SQLITE_OPEN_READWRITE,NULL))
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

	m_del(db_path);

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
static Return read_cmpctstat_from_db(
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
	m_create(char,db_path,MEMORY_STRING);

	if(SUCCESS == status && (db_filename == NULL || relative_path == NULL || stat_out == NULL))
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
		int rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW)
		{
			const void *blob = sqlite3_column_blob(stmt,0);
			int bytes = sqlite3_column_bytes(stmt,0);

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

	m_del(db_path);

	return(status);
}

/**
 * @brief Check whether DB compact-stat timestamps match the current file stat timestamps
 *
 * @param[in] db_stat Compact stat loaded from the database
 * @param[in] file_stat Current filesystem stat structure
 * @return `true` when both ctime and mtime fields match exactly, otherwise `false`
 */
static bool cmpctstat_matches_stat_timestamps(
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
 * @brief Verify that a checksum-locked file size change is reported but not saved
 *
 * Covers README Example 10, case 1
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After one locked file grows on disk, the update run with `--rehash-locked`
 * must warn about possible data corruption and keep the original database row
 */
static Return test0030_1(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output and
	 * the expected regular-expression template used by this test
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * that will be changed on disk during the second half of the test
	 */
	const char *db_filename = "lock_s1.db";
	const char *locked_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the protected database row.
	 * They prove that a warning result did not silently update locked data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s1.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before corrupting the on-disk file.
	 * These values are expected to remain unchanged after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Grow one checksum-locked file on disk.
	 * This creates a size mismatch against the protected database row
	 */
	ASSERT(SUCCESS == write_string_to_file("pad",locked_file_path,FILE_WRITE_APPEND));

	/*
	 * Run an update with deep locked-file verification enabled.
	 * The size mismatch must be reported as a warning, not saved to the DB
	 */
	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output.
	 * The changed locked file must be reported as possible data corruption
	 */
	filename = "templates/0030_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that watched timestamp drift on a locked file is reported but not saved
 *
 * Covers README Example 10, case 4
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After one locked file gets a metadata-only timestamp change, the update run
 * with `--watch-timestamps` must warn and keep the original database row
 */
static Return test0030_2(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output and
	 * the expected regular-expression template used by this test
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose timestamps will be changed without touching file contents
	 */
	const char *db_filename = "lock_s2.db";
	const char *locked_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the protected database row.
	 * They prove that a warning result did not silently update locked data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s2.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before changing file timestamps.
	 * These values are expected to remain unchanged after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Change only timestamps for one checksum-locked file.
	 * The file size and contents stay unchanged, but ctime and mtime drift
	 */
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,locked_file_path,999));

	/*
	 * Run an update that treats timestamp drift as meaningful.
	 * The drift must be reported as a warning, not saved to the DB
	 */
	arguments = "--update --watch-timestamps --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output.
	 * The changed locked file must be reported with ctime and mtime drift
	 */
	filename = "templates/0030_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that locked timestamp drift is ignored when timestamp watching is off
 *
 * Covers README Example 10, case 2
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After one locked file gets a metadata-only timestamp change, the update run
 * without `--watch-timestamps` or `--rehash-locked` must finish successfully
 * and keep the original database row
 */
static Return test0030_3(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved fixture path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose timestamps will be changed without touching file contents
	 */
	const char *db_filename = "lock_s3.db";
	const char *locked_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the protected database row.
	 * They prove that ignored timestamp drift did not silently update locked data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	struct stat file_stat_before = {0};
	struct stat file_stat_after = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s3.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the changed fixture path and read its matching locked DB row.
	 * Before any drift, the file timestamps should match the stored metadata
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_before));
	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_before));

	/*
	 * Change only timestamps for one checksum-locked file.
	 * The file size and contents stay unchanged, but ctime and mtime drift
	 */
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,locked_file_path,999));

	/*
	 * Confirm that the test setup produced real timestamp drift.
	 * The stored DB metadata must no longer match the current file timestamps
	 */
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_after));
	ASSERT(false == cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_after));

	/*
	 * Run an update without timestamp watching or deep locked-file rehashing.
	 * The timestamp drift must be ignored and the run must finish successfully
	 */
	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s3.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the successful output.
	 * It must report that no database changes were needed
	 */
	filename = "templates/0030_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the successful update run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that locked timestamp drift is safely synchronized after rehash
 *
 * Covers README Example 10, case 5
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After one locked file gets a metadata-only timestamp change, the update run
 * with `--watch-timestamps` and `--rehash-locked` must rehash the file,
 * finish successfully, and store the new timestamps without changing its checksum
 */
static Return test0030_4(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved fixture path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose timestamps will be changed without touching file contents
	 */
	const char *db_filename = "lock_s4.db";
	const char *locked_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep file and database snapshots around the timestamp drift.
	 * They prove that the drift is real and later synchronized to the database
	 */
	struct stat file_stat_before = {0};
	struct stat file_stat_drift = {0};
	struct stat file_stat_after = {0};
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s4.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the changed fixture path and read its matching locked DB row.
	 * Before any drift, the file timestamps should match the stored metadata
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_before));
	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_before));

	/*
	 * Change only timestamps for one checksum-locked file.
	 * The file size and contents stay unchanged, but ctime and mtime drift
	 */
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,locked_file_path,999));

	/*
	 * Confirm that the test setup produced real timestamp drift.
	 * The stored DB metadata must no longer match the current file timestamps
	 */
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_drift));
	ASSERT(false == cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_drift));

	/*
	 * Run an update that watches timestamps and rehashes locked files.
	 * The checksum match must make the timestamp-only drift safe to save
	 */
	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s4.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the successful update output.
	 * The changed locked file must be reported as rehashed and synchronized
	 */
	filename = "templates/0030_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the protected row and current file state after the update.
	 * The database timestamps must now match the file again
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_after));
	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_after,&file_stat_after));

	/*
	 * Verify that only metadata timestamps changed in the stored row.
	 * Size, allocation, identity, SHA512 offset, and SHA512 digest must stay stable
	 */
	ASSERT(db_stat_before.st_size == db_stat_after.st_size);
	ASSERT(db_stat_before.st_blocks == db_stat_after.st_blocks);
	ASSERT(db_stat_before.st_dev == db_stat_after.st_dev);
	ASSERT(db_stat_before.st_ino == db_stat_after.st_ino);
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that locked timestamp drift is synchronized by rehash alone
 *
 * Covers README Example 10, case 5
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After one locked file gets a metadata-only timestamp change, the update run
 * with `--rehash-locked` and without `--watch-timestamps` must rehash the file,
 * finish successfully, and store the new timestamps without changing its checksum
 */
static Return test0030_5(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved fixture path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose timestamps will be changed without touching file contents
	 */
	const char *db_filename = "lock_s5.db";
	const char *locked_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep file and database snapshots around the timestamp drift.
	 * They prove that the drift is real and later synchronized to the database
	 */
	struct stat file_stat_before = {0};
	struct stat file_stat_drift = {0};
	struct stat file_stat_after = {0};
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s5.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_005_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the changed fixture path and read its matching locked DB row.
	 * Before any drift, the file timestamps should match the stored metadata
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_before));
	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_before));

	/*
	 * Change only timestamps for one checksum-locked file.
	 * The file size and contents stay unchanged, but ctime and mtime drift
	 */
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,locked_file_path,999));

	/*
	 * Confirm that the test setup produced real timestamp drift.
	 * The stored DB metadata must no longer match the current file timestamps
	 */
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_drift));
	ASSERT(false == cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_drift));

	/*
	 * Run an update that rehashes locked files without timestamp watching.
	 * The checksum match must make the timestamp-only drift safe to save
	 */
	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s5.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the successful update output.
	 * All locked files must be reported as successfully rehashed
	 */
	filename = "templates/0030_005_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the protected row and current file state after the update.
	 * The database timestamps must now match the file again
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_after));
	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_after,&file_stat_after));

	/*
	 * Verify that only metadata timestamps changed in the stored row.
	 * Size, allocation, identity, SHA512 offset, and SHA512 digest must stay stable
	 */
	ASSERT(db_stat_before.st_size == db_stat_after.st_size);
	ASSERT(db_stat_before.st_blocks == db_stat_after.st_blocks);
	ASSERT(db_stat_before.st_dev == db_stat_after.st_dev);
	ASSERT(db_stat_before.st_ino == db_stat_after.st_ino);
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify mixed locked-file results during a watched rehash audit
 *
 * Covers README Example 10
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * One locked file then receives a metadata-only timestamp change, while another
 * locked file keeps its on-disk contents but gets a corrupted stored SHA512 value.
 * The update run with `--watch-timestamps` and `--rehash-locked` must synchronize
 * the safe timestamp drift, report the checksum mismatch as corruption, and leave
 * the mismatched DB checksum untouched
 */
static Return test0030_6(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and resolved fixture paths
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,drift_target_path,MEMORY_STRING);
	m_create(char,mismatch_target_path,MEMORY_STRING);

	/*
	 * Name the database, the locked file that will drift only by timestamps,
	 * and the locked file whose stored checksum will be corrupted in the DB
	 */
	const char *db_filename = "lock_s6.db";
	const char *drift_relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *drift_file_path = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";
	const char *mismatch_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *mismatch_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";

	/*
	 * Keep file and database snapshots for the timestamp-drift row.
	 * They prove that the safe drift is saved without changing the checksum
	 */
	struct stat drift_file_stat_before = {0};
	struct stat drift_file_stat_after_touch = {0};
	struct stat drift_file_stat_after_update = {0};
	CmpctStat drift_db_stat_before = {0};
	CmpctStat drift_db_stat_after = {0};
	sqlite3_int64 drift_offset_before = 0;
	sqlite3_int64 drift_offset_after = 0;
	unsigned char drift_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char drift_sha512_after[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Keep checksum snapshots for the mismatch row.
	 * They prove that a detected corruption warning does not silently repair the DB
	 */
	CmpctStat mismatch_db_stat_before = {0};
	CmpctStat mismatch_db_stat_after = {0};
	sqlite3_int64 mismatch_offset_before = 0;
	sqlite3_int64 mismatch_offset_tampered = 0;
	sqlite3_int64 mismatch_offset_after = 0;
	unsigned char mismatch_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char mismatch_sha512_tampered[SHA512_DIGEST_LENGTH] = {0};
	unsigned char mismatch_sha512_after[SHA512_DIGEST_LENGTH] = {0};
	unsigned char mismatch_file_sha512[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s6.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_006_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve fixture paths and read the rows that will be exercised.
	 * The baseline checksum for the mismatch row must match the real file
	 */
	ASSERT(SUCCESS == construct_path(drift_file_path,drift_target_path));
	ASSERT(SUCCESS == construct_path(mismatch_file_path,mismatch_target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,drift_relative_path,&drift_db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,drift_relative_path,&drift_offset_before,drift_sha512_before));
	ASSERT(SUCCESS == get_file_stat(m_text(drift_target_path),&drift_file_stat_before));
	ASSERT(cmpctstat_matches_stat_timestamps(&drift_db_stat_before,&drift_file_stat_before));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,mismatch_relative_path,&mismatch_db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,mismatch_relative_path,&mismatch_offset_before,mismatch_sha512_before));
	ASSERT(SUCCESS == compute_file_sha512(m_text(mismatch_target_path),mismatch_file_sha512));
	ASSERT(0 == memcmp(mismatch_sha512_before,mismatch_file_sha512,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Change only timestamps for one checksum-locked file.
	 * The file size and contents stay unchanged, but ctime and mtime drift
	 */
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,drift_file_path,999));

	/*
	 * Confirm that the test setup produced real timestamp drift.
	 * The stored DB metadata must no longer match the current file timestamps
	 */
	ASSERT(SUCCESS == get_file_stat(m_text(drift_target_path),&drift_file_stat_after_touch));
	ASSERT(false == cmpctstat_matches_stat_timestamps(&drift_db_stat_before,&drift_file_stat_after_touch));

	/*
	 * Corrupt the stored checksum for a different locked file.
	 * The file on disk stays unchanged, so the next rehash must detect a mismatch
	 */
	ASSERT(SUCCESS == db_tamper_locked_checksum(db_filename,mismatch_relative_path));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,mismatch_relative_path,&mismatch_offset_tampered,mismatch_sha512_tampered));
	ASSERT(mismatch_offset_before == mismatch_offset_tampered);
	ASSERT(0 != memcmp(mismatch_sha512_before,mismatch_sha512_tampered,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(mismatch_file_sha512,mismatch_sha512_tampered,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Run an update that watches timestamps and rehashes locked files.
	 * The safe drift must be saved, while the checksum mismatch must warn
	 */
	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s6.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real scenario.
	 * It must show synchronized timestamp drift and a separate checksum mismatch
	 */
	filename = "templates/0030_006_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the drift row and current file state after the warning run.
	 * The database timestamps must now match the file again
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,drift_relative_path,&drift_db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,drift_relative_path,&drift_offset_after,drift_sha512_after));
	ASSERT(SUCCESS == get_file_stat(m_text(drift_target_path),&drift_file_stat_after_update));
	ASSERT(cmpctstat_matches_stat_timestamps(&drift_db_stat_after,&drift_file_stat_after_update));

	/*
	 * Verify that only metadata timestamps changed in the safely rehashed row.
	 * Size, allocation, identity, SHA512 offset, and SHA512 digest must stay stable
	 */
	ASSERT(drift_db_stat_before.st_size == drift_db_stat_after.st_size);
	ASSERT(drift_db_stat_before.st_blocks == drift_db_stat_after.st_blocks);
	ASSERT(drift_db_stat_before.st_dev == drift_db_stat_after.st_dev);
	ASSERT(drift_db_stat_before.st_ino == drift_db_stat_after.st_ino);
	ASSERT(drift_offset_before == drift_offset_after);
	ASSERT(0 == memcmp(drift_sha512_before,drift_sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Re-read the mismatched row after the warning run.
	 * The corrupted stored checksum must remain unchanged instead of being repaired
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,mismatch_relative_path,&mismatch_db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,mismatch_relative_path,&mismatch_offset_after,mismatch_sha512_after));
	ASSERT(0 == memcmp(&mismatch_db_stat_before,&mismatch_db_stat_after,sizeof(CmpctStat)));
	ASSERT(mismatch_offset_tampered == mismatch_offset_after);
	ASSERT(0 == memcmp(mismatch_sha512_tampered,mismatch_sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(mismatch_file_sha512,mismatch_sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(mismatch_target_path);
	m_del(drift_target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that locked content corruption is reported but not saved
 *
 * Covers README Example 10
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After bytes inside one locked file change on disk, the update run with
 * `--rehash-locked` must detect the checksum mismatch, report corruption,
 * and keep the original database row untouched
 */
static Return test0030_7(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved fixture path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose on-disk bytes will be changed without changing its logical size
	 */
	const char *db_filename = "lock_s7.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";

	/*
	 * Keep file and database snapshots around the content tampering.
	 * They prove that the warning run does not silently trust corrupted data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char db_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char db_sha512_after[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_after_tamper[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s7.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_007_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the target fixture path and read its matching locked DB row.
	 * Before tampering, the stored checksum must match the real file checksum
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,db_sha512_before));
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_before));
	ASSERT(0 == memcmp(db_sha512_before,file_sha512_before,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Change bytes inside one checksum-locked file on disk.
	 * The helper preserves logical size, so the mismatch is about content integrity
	 */
	ASSERT(SUCCESS == tamper_locked_file_bytes(locked_file_path));

	/*
	 * Confirm that the test setup produced real content corruption.
	 * The current file checksum must no longer match the protected DB checksum
	 */
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_after_tamper));
	ASSERT(0 != memcmp(file_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Run an update with deep locked-file verification enabled.
	 * The content mismatch must be reported as a warning, not saved to the DB
	 */
	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s7.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real scenario.
	 * The changed locked file must be reported as checksum corruption
	 */
	filename = "templates/0030_007_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,db_sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(db_sha512_before,db_sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_after,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that watched locked content corruption is reported but not saved
 *
 * Covers README Example 10
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * After bytes inside one locked file change on disk, the update run with
 * `--watch-timestamps` and `--rehash-locked` must detect checksum corruption.
 * Timestamp watching must not make the corrupted database row updateable
 */
static Return test0030_8(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved fixture path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose on-disk bytes will be changed without changing its logical size
	 */
	const char *db_filename = "lock_s8.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";

	/*
	 * Keep file and database snapshots around the content tampering.
	 * They prove that the warning run does not silently trust corrupted data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char db_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char db_sha512_after[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_after_tamper[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s8.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_008_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the target fixture path and read its matching locked DB row.
	 * Before tampering, the stored checksum must match the real file checksum
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,db_sha512_before));
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_before));
	ASSERT(0 == memcmp(db_sha512_before,file_sha512_before,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Change bytes inside one checksum-locked file on disk.
	 * The helper preserves logical size, so the mismatch is about content integrity
	 */
	ASSERT(SUCCESS == tamper_locked_file_bytes(locked_file_path));

	/*
	 * Confirm that the test setup produced real content corruption.
	 * The current file checksum must no longer match the protected DB checksum
	 */
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_after_tamper));
	ASSERT(0 != memcmp(file_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Run an update that watches timestamps and rehashes locked files.
	 * The content mismatch must be reported as a warning, not saved to the DB
	 */
	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s8.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real scenario.
	 * The changed locked file must be reported as checksum corruption
	 */
	filename = "templates/0030_008_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,db_sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(db_sha512_before,db_sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_after,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that watched unchanged locked files finish successfully
 *
 * Covers README Example 10, case 3
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * When the locked files still match the stored metadata and `--rehash-locked`
 * is not used, `--watch-timestamps` must not create warnings or updates.
 * All protected rows must stay unchanged in the database
 */
static Return test0030_9(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and reusable resolved paths
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database and every file protected by the lock pattern.
	 * This test checks all protected rows, because no locked path should change
	 */
	const char *db_filename = "lock_s9.db";
	const char *locked_relative_paths[] = {
		"path1/AAA/BCB/CCC/a.txt",
		"path1/AAA/ZAW/A/b/c/a_file.txt",
		"path1/AAA/ZAW/D/e/f/b_file.txt",
	};
	const char *locked_file_paths[] = {
		"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",
		"tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt",
		"tests/fixtures/diffs/diff1/path1/AAA/ZAW/D/e/f/b_file.txt",
	};
	const size_t locked_path_count = sizeof(locked_relative_paths) / sizeof(locked_relative_paths[0]);

	/*
	 * Keep before-and-after snapshots of every protected database row.
	 * They prove that a successful watched update did not rewrite locked data
	 */
	CmpctStat db_stat_before[3] = {0};
	CmpctStat db_stat_after[3] = {0};
	struct stat file_stat_before[3] = {0};
	struct stat file_stat_after[3] = {0};
	sqlite3_int64 offset_before[3] = {0};
	sqlite3_int64 offset_after[3] = {0};
	unsigned char sha512_before[3][SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[3][SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s9.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_009_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read all protected rows and current file timestamps before the update.
	 * For this README case, every locked file must still match the database
	 */
	for(size_t index = 0; index < locked_path_count; index++)
	{
		ASSERT(SUCCESS == construct_path(locked_file_paths[index],target_path));
		ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_paths[index],&db_stat_before[index]));
		ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_paths[index],&offset_before[index],sha512_before[index]));
		ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_before[index]));
		ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before[index],&file_stat_before[index]));
	}

	/*
	 * Run an update that watches timestamps but does not rehash locked files.
	 * Because all protected metadata still matches, the run must be successful
	 */
	arguments = "--update --watch-timestamps --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s9.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the successful output.
	 * It must report no changes and no locked-file warnings
	 */
	filename = "templates/0030_009_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read all protected rows after the successful watched update.
	 * Metadata, SHA512 offsets, and SHA512 digests must remain unchanged
	 */
	for(size_t index = 0; index < locked_path_count; index++)
	{
		ASSERT(SUCCESS == construct_path(locked_file_paths[index],target_path));
		ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_paths[index],&db_stat_after[index]));
		ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_paths[index],&offset_after[index],sha512_after[index]));
		ASSERT(SUCCESS == get_file_stat(m_text(target_path),&file_stat_after[index]));
		ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_after[index],&file_stat_after[index]));
		ASSERT(0 == memcmp(&db_stat_before[index],&db_stat_after[index],sizeof(CmpctStat)));
		ASSERT(offset_before[index] == offset_after[index]);
		ASSERT(0 == memcmp(sha512_before[index],sha512_after[index],(size_t)SHA512_DIGEST_LENGTH));
	}

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that deleting a locked file warns and keeps the DB row
 *
 * Covers README Example 9
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * When a locked file disappears before an update, the program must warn and
 * keep every locked row in the database. The deleted row must remain unchanged
 */
static Return test0030_10(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for the captured application output,
	 * the expected regular-expression template, and the resolved deleted path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the deleted locked row, and the matching fixture file.
	 * The other locked rows are checked separately to prove the whole lock set survives
	 */
	const char *db_filename = "lock_s10.db";
	const char *deleted_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *deleted_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the deleted locked row.
	 * They prove that reporting the missing file did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s10.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_010_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before deleting its file.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_before,sha512_before));
	ASSERT(SUCCESS == construct_path(deleted_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Delete one checksum-locked file from disk.
	 * This must be treated as a lock violation, not as a normal missing-file cleanup
	 */
	ASSERT(SUCCESS == delete_path(deleted_file_path));
	ASSERT(FILE_NOT_FOUND == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update with the same lock pattern.
	 * The missing locked file must be reported as a warning and kept in the DB
	 */
	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s10.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real scenario.
	 * The deleted locked file must be reported as disappeared from disk
	 */
	filename = "templates/0030_010_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the deleted file row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * A missing locked file must not cause any protected row to be dropped
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,deleted_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --db-drop-inaccessible does not drop an access-denied locked file
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then a test hook makes one locked file look access-denied during an update run
 * with `--db-drop-inaccessible`. The program must warn about the locked file
 * and keep the protected database row unchanged
 */
static Return test0030_11(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved locked file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the access-denied locked row, and the matching
	 * fixture file. The other locked rows prove that the whole lock set survives
	 */
	const char *db_filename = "lock_s11.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the access-denied locked row.
	 * They prove that a warning result did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s11.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_011_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making access checks fail.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers an access-denied report, not a missing-file scenario
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Force access checks for exactly this locked file to return access denied.
	 * This models a protected file that the application can see in the tree but cannot read
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_DENIED"));
	ASSERT(FILE_ACCESS_DENIED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that would normally drop inaccessible records.
	 * The checksum lock must override that cleanup and turn the condition into a warning
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s11.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable the access-check hook before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The run must report checksum-locked access denial and return WARNING
	 */
	filename = "templates/0030_011_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-denied locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-inaccessible must not drop any checksum-locked row
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --db-drop-inaccessible does not drop a locked file after an access-check failure
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then a test hook makes one locked file return an access-check failure during
 * an update run with `--db-drop-inaccessible`. The program must warn about the
 * locked file and keep the protected database row unchanged
 */
static Return test0030_12(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved locked file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the locked row that will fail access checking, and
	 * the matching fixture file. The other locked rows prove that the whole lock set survives
	 */
	const char *db_filename = "lock_s12.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the locked row that will fail access checking.
	 * They prove that a warning result did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s12.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_012_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making access checks fail.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers an access-check failure, not a missing-file scenario
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Force access checks for exactly this locked file to return an access-check failure.
	 * This models an unexpected access-check problem while the checksum lock is active
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(FILE_ACCESS_ERROR == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that would normally drop inaccessible records.
	 * The checksum lock must override that cleanup and turn the condition into a warning
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s12.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable the access-check hook before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The run must report checksum-locked access-check failure and return WARNING
	 */
	filename = "templates/0030_012_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-check-failed locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-inaccessible must not drop any checksum-locked row
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --db-drop-ignored does not drop a deleted checksum-locked file
 *
 * Covers README Example 9
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then one locked file is deleted and the update run ignores the whole protected
 * subtree with `--db-drop-ignored`. The program must warn about the missing
 * locked file and keep the protected database row unchanged
 */
static Return test0030_13(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved deleted file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the deleted locked row, and the matching fixture file.
	 * The other locked rows prove that the whole lock set survives
	 */
	const char *db_filename = "lock_s13.db";
	const char *deleted_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *deleted_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the deleted locked row.
	 * They prove that a warning result did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s13.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_013_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before deleting its file.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file exists, then delete it from disk.
	 * The later update must treat this as a checksum-lock violation, not ignored cleanup
	 */
	ASSERT(SUCCESS == construct_path(deleted_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));
	ASSERT(SUCCESS == delete_path(deleted_file_path));
	ASSERT(FILE_NOT_FOUND == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that ignores the whole protected subtree and allows ignored DB cleanup.
	 * The checksum lock must override --db-drop-ignored for the missing protected file
	 */
	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --db-drop-ignored "
	        "--database=lock_s13.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The deleted locked file must be reported as disappeared and kept in the DB
	 */
	filename = "templates/0030_013_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the deleted locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-ignored must not drop any checksum-locked row
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,deleted_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --rehash-locked still checks ignored checksum-locked files
 *
 * Covers README Examples 9 and 10
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then one locked file is changed on disk and the update run ignores the whole
 * protected subtree. `--rehash-locked` must still rehash the locked files,
 * report the corrupted file, and keep the protected database row unchanged
 */
static Return test0030_14(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved corrupted file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose on-disk bytes will be changed while the path is also ignored
	 */
	const char *db_filename = "lock_s14.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";

	/*
	 * Keep file and database snapshots around the content tampering.
	 * They prove that ignored locked corruption is detected but not saved
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char db_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char db_sha512_after[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_after_tamper[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s14.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_014_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the target fixture path and read its matching locked DB row.
	 * Before tampering, the stored checksum must match the real file checksum
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,db_sha512_before));
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_before));
	ASSERT(0 == memcmp(db_sha512_before,file_sha512_before,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Change bytes inside one checksum-locked file on disk.
	 * The helper preserves logical size, so the mismatch is about content integrity
	 */
	ASSERT(SUCCESS == tamper_locked_file_bytes(locked_file_path));

	/*
	 * Confirm that the test setup produced real content corruption.
	 * The current file checksum must no longer match the protected DB checksum
	 */
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_after_tamper));
	ASSERT(0 != memcmp(file_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Run an update that ignores the protected subtree but rehashes locked files.
	 * The ignored corrupted file must still be reported as a checksum violation
	 */
	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --database=lock_s14.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * Ignored directories are reported, but locked files are still rehashed
	 */
	filename = "templates/0030_014_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,db_sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(db_sha512_before,db_sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_after,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --include does not weaken checksum-lock cleanup protection
 *
 * Covers README Example 9
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then one locked file is deleted while the update run ignores the whole
 * protected subtree and restores only a different locked file through `--include`.
 * `--db-drop-ignored` must not remove the missing locked row: the program must
 * report a warning and keep the protected database row unchanged
 */
static Return test0030_15(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved deleted file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the deleted protected row, and the other locked rows
	 * that must stay in the database after ignored cleanup
	 */
	const char *db_filename = "lock_s15.db";
	const char *deleted_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *deleted_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *included_locked_relative_path = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep a before/after snapshot of the deleted locked row.
	 * These values prove that warning cleanup did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s15.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_015_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before deleting its file.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file exists, then delete it from disk.
	 * The later update must treat this as a checksum-lock violation, not ignored cleanup
	 */
	ASSERT(SUCCESS == construct_path(deleted_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));
	ASSERT(SUCCESS == delete_path(deleted_file_path));
	ASSERT(FILE_NOT_FOUND == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that ignores the protected subtree, restores one different
	 * locked file through --include, and allows ignored DB cleanup.
	 * The deleted locked row is outside the restored include subset, but the
	 * checksum lock must still override --db-drop-ignored
	 */
	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --include=\"^path1/AAA/BCB/CCC/a\\.txt$\" "
	        "--db-drop-ignored --database=lock_s15.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The deleted locked file must be reported as disappeared and kept in the DB
	 */
	filename = "templates/0030_015_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the deleted locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,deleted_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,deleted_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the deleted locked row, another ignored locked row, and the
	 * included locked row all remain in the database
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,deleted_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,included_locked_relative_path,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --rehash-locked checks locked files outside the restored --include subset
 *
 * Covers README Examples 9 and 10
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then one locked file is changed on disk while the update run ignores the whole
 * protected subtree and restores only a different locked file through `--include`.
 * `--rehash-locked` must still rehash the corrupted locked file, report a warning,
 * and keep the protected database row unchanged
 */
static Return test0030_16(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved corrupted file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose on-disk bytes will be changed outside the restored include subset
	 */
	const char *db_filename = "lock_s16.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";

	/*
	 * Keep file and database snapshots around the content tampering.
	 * They prove that ignored locked corruption is detected but not saved
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	struct stat file_stat_before = {0};
	struct stat file_stat_after_tamper = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char db_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char db_sha512_after[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char file_sha512_after_tamper[SHA512_DIGEST_LENGTH] = {0};

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s16.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_016_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Resolve the target fixture path and read its matching locked DB row.
	 * Before tampering, the stored checksum must match the real file checksum
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(0 == stat(m_text(target_path),&file_stat_before));
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,db_sha512_before));
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_before));
	ASSERT(0 == memcmp(db_sha512_before,file_sha512_before,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Change bytes inside one checksum-locked file on disk.
	 * The helper preserves logical size, so the mismatch is about content integrity
	 */
	ASSERT(SUCCESS == tamper_locked_file_bytes(locked_file_path));

	/*
	 * Confirm that the test setup produced real content corruption.
	 * The file size must stay the same, while the checksum must differ
	 */
	ASSERT(0 == stat(m_text(target_path),&file_stat_after_tamper));
	ASSERT(file_stat_before.st_size == file_stat_after_tamper.st_size);
	ASSERT(SUCCESS == compute_file_sha512(m_text(target_path),file_sha512_after_tamper));
	ASSERT(0 != memcmp(file_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_before,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Run an update that ignores the protected subtree, restores one different
	 * locked file through --include, and rehashes locked files.
	 * The corrupted locked row is outside the restored include subset, but
	 * --rehash-locked must still check it and report a checksum violation
	 */
	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --include=\"^path1/AAA/BCB/CCC/a\\.txt$\" "
	        "--database=lock_s16.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * Ignored directories are reported, but locked files are still rehashed
	 */
	filename = "templates/0030_016_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the same protected row after the warning run.
	 * The row must keep its original metadata, offset, and SHA512 checksum
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,db_sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(db_sha512_before,db_sha512_after,(size_t)SHA512_DIGEST_LENGTH));
	ASSERT(0 != memcmp(db_sha512_after,file_sha512_after_tamper,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --ignore does not hide access denial for a checksum-locked file
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then a test hook makes one locked file look access-denied during an update run
 * that also ignores the protected subtree and enables `--db-drop-inaccessible`.
 * The program must warn about the ignored locked file and keep the protected
 * database row unchanged
 */
static Return test0030_17(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved locked file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the access-denied locked row, and the matching fixture file.
	 * The other locked rows prove that the whole lock set survives ignored cleanup
	 */
	const char *db_filename = "lock_s17.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the access-denied locked row.
	 * They prove that a warning result did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s17.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_017_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making access checks fail.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers access denial for an existing ignored locked file
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Force access checks for exactly this locked file to return access denied.
	 * This models a visible protected file that cannot be read during traversal
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_DENIED"));
	ASSERT(FILE_ACCESS_DENIED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that ignores the protected subtree and would normally drop
	 * inaccessible records. The checksum lock must override both conditions
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --database=lock_s17.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable the access-check hook before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The ignored locked file must still report checksum-locked access denial
	 */
	filename = "templates/0030_017_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-denied locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --ignore and --db-drop-inaccessible must not drop any checksum-locked row
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that --ignore does not hide access-check failure for a checksum-locked file
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with the `path1/` subtree protected by `--lock-checksum`.
 * Then a test hook makes one locked file return an access-check failure during
 * an update run that also ignores the protected subtree and enables
 * `--db-drop-inaccessible`. The program must warn about the ignored locked file
 * and keep the protected database row unchanged
 */
static Return test0030_18(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved locked file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the locked row that will fail access checking, and the
	 * matching fixture file. The other locked rows prove that the whole lock set survives ignored cleanup
	 */
	const char *db_filename = "lock_s18.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep before-and-after snapshots of the locked row that will fail access checking.
	 * They prove that a warning result did not rewrite protected data
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s18.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_018_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making access checks fail.
	 * The same values must still be present after the warning run
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers an access-check failure for an existing ignored locked file
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Force access checks for exactly this locked file to return an access-check failure.
	 * This models an unexpected access-check problem during ignored locked traversal
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(FILE_ACCESS_ERROR == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that ignores the protected subtree and would normally drop
	 * inaccessible records. The checksum lock must override both conditions
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --database=lock_s18.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable the access-check hook before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * The ignored locked file must still report checksum-locked access-check failure
	 */
	filename = "templates/0030_018_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-check-failed locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --ignore and --db-drop-inaccessible must not drop any checksum-locked row
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that cleanup keeps an access-denied checksum-locked row
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with `path1/` protected by `--lock-checksum`.
 * Then the test makes one locked file unreadable and skips `file_list`, so
 * `db_delete_missing_metadata` must handle the unavailable DB row directly.
 * Even with `--db-drop-inaccessible`, the checksum lock must report a warning
 * and keep the protected database row unchanged
 */
static Return test0030_19(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved access-denied file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose read access will be denied during cleanup-only processing
	 */
	const char *db_filename = "lock_s19.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep database snapshots around the warning run.
	 * They prove that cleanup reports the access denial but does not rewrite the locked row
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s19.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_019_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making its file unreadable.
	 * The same values must still be present after cleanup reports the warning
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file exists and is readable, then deny read access.
	 * This makes the later cleanup pass handle a real access-denied DB row
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));
	ASSERT(SUCCESS == change_mode(locked_file_path,0000));
	ASSERT(FILE_ACCESS_DENIED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Skip both file_list passes.
	 * This forces db_delete_missing_metadata to be the code path that checks the locked DB row
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST","1"));

	/*
	 * Run an update that would normally drop inaccessible records.
	 * The checksum lock must override that cleanup and turn the condition into a warning
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s19.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Re-enable file_list and restore file permissions before inspecting state.
	 * The captured output still reflects the access-denied cleanup run above
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST",""));
	ASSERT(SUCCESS == change_mode(locked_file_path,0644));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * With file_list bypassed, db_delete_missing_metadata must report access denied
	 */
	filename = "templates/0030_019_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-denied locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-inaccessible must not drop any checksum-locked row during cleanup
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that cleanup keeps a checksum-locked row after an access-check failure
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with `path1/` protected by `--lock-checksum`.
 * Then the test makes access checking fail for one locked file and skips
 * `file_list`, so `db_delete_missing_metadata` must handle the unavailable DB
 * row directly. Even with `--db-drop-inaccessible`, the checksum lock must
 * report a warning and keep the protected database row unchanged
 */
static Return test0030_20(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved access-failed file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose access check will fail during cleanup-only processing
	 */
	const char *db_filename = "lock_s20.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep database snapshots around the warning run.
	 * They prove that cleanup reports the access-check failure but does not rewrite the locked row
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s20.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_020_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making its access check fail.
	 * The same values must still be present after cleanup reports the warning
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers an access-check failure for an existing DB row
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Skip both file_list passes and force access checks for exactly this
	 * locked file to return an access-check failure
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST","1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(FILE_ACCESS_ERROR == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that would normally drop inaccessible records.
	 * The checksum lock must override that cleanup and turn the condition into a warning
	 */
	arguments = "--update --db-drop-inaccessible --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s20.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable test hooks before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * With file_list bypassed, db_delete_missing_metadata must report access-check failure
	 */
	filename = "templates/0030_020_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the access-check-failed locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-inaccessible must not drop any checksum-locked row during cleanup
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Verify that cleanup keeps an ignored checksum-locked row after an access-check failure
 *
 * Covers README Examples 9 and 11
 *
 * Creates a baseline database with `path1/` protected by `--lock-checksum`.
 * Then the test makes access checking fail for one locked file and skips
 * `file_list`, so `db_delete_missing_metadata` must handle an ignored DB row
 * directly. Even with `--ignore` and `--db-drop-ignored`, the checksum lock
 * must report a warning and keep the protected database row unchanged
 */
static Return test0030_21(void)
{
	INITTEST;

	/*
	 * Allocate managed buffers for captured application output, the expected
	 * regular-expression template, and the resolved access-failed file path
	 */
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,target_path,MEMORY_STRING);

	/*
	 * Name the database, the protected database key, and the fixture file
	 * whose access check will fail during ignored cleanup-only processing
	 */
	const char *db_filename = "lock_s21.db";
	const char *locked_relative_path = "path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *locked_file_path = "tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt";
	const char *other_locked_relative_path_1 = "path1/AAA/ZAW/D/e/f/b_file.txt";
	const char *other_locked_relative_path_2 = "path1/AAA/BCB/CCC/a.txt";

	/*
	 * Keep database snapshots around the warning run.
	 * They prove that ignored cleanup reports the access-check failure but does not rewrite the locked row
	 */
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};
	sqlite3_int64 offset_before = 0;
	sqlite3_int64 offset_after = 0;
	unsigned char sha512_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char sha512_after[SHA512_DIGEST_LENGTH] = {0};
	bool row_exists = false;

	/*
	 * Enable deterministic TESTING output so the captured application log
	 * can be compared against a template
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	/*
	 * Work on a mutable copy of the fixture tree.
	 * The original fixture is restored during cleanup
	 */
	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Create the baseline database.
	 * The lock pattern protects every relative path under the path1/ subtree
	 */
	const char *arguments = "--database=lock_s21.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	/*
	 * Verify the baseline run output.
	 * It must show a new database with path1/ files recorded as checksum-locked
	 */
	const char *filename = "templates/0030_021_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Read the locked row before making its access check fail.
	 * The same values must still be present after ignored cleanup reports the warning
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_before));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_before,sha512_before));

	/*
	 * Confirm the fixture file itself is present and readable.
	 * This test covers an access-check failure for an existing ignored DB row
	 */
	ASSERT(SUCCESS == construct_path(locked_file_path,target_path));
	ASSERT(FILE_ACCESS_ALLOWED == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Skip both file_list passes and force access checks for exactly this
	 * locked file to return an access-check failure
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST","1"));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",locked_file_path));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(FILE_ACCESS_ERROR == file_check_access(m_text(target_path),target_path->string_length,R_OK));

	/*
	 * Run an update that would normally drop ignored records.
	 * The checksum lock must override that cleanup and turn the condition into a warning
	 */
	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--ignore=\"^path1/.*\" --db-drop-ignored "
	        "--database=lock_s21.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	/*
	 * Disable test hooks before inspecting output and database state.
	 * Later assertions should observe the real filesystem again
	 */
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_SKIP_FILE_LIST",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	/*
	 * Verify the warning output against the real command-line scenario.
	 * With file_list bypassed, ignored cleanup must report access-check failure
	 */
	filename = "templates/0030_021_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	/*
	 * Re-read the ignored access-check-failed locked row after the warning run.
	 * Its metadata, SHA512 offset, and SHA512 digest must stay unchanged
	 */
	ASSERT(SUCCESS == read_cmpctstat_from_db(db_filename,locked_relative_path,&db_stat_after));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,locked_relative_path,&offset_after,sha512_after));
	ASSERT(0 == memcmp(&db_stat_before,&db_stat_after,sizeof(CmpctStat)));
	ASSERT(offset_before == offset_after);
	ASSERT(0 == memcmp(sha512_before,sha512_after,(size_t)SHA512_DIGEST_LENGTH));

	/*
	 * Confirm that the full locked path set remains in the database.
	 * --db-drop-ignored must not drop any checksum-locked row during cleanup
	 */
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,locked_relative_path,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_1,&row_exists));
	ASSERT(row_exists == true);
	ASSERT(SUCCESS == db_relative_path_exists(db_filename,other_locked_relative_path_2,&row_exists));
	ASSERT(row_exists == true);

	/*
	 * Remove the test database and restore the mutable fixture.
	 * This returns the workspace to the state expected by the next test
	 */
	ASSERT(SUCCESS == delete_path(db_filename));
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/*
	 * Release managed buffers allocated by this test before returning
	 */
	m_del(target_path);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Run the README lock-checksum scenario tests
 *
 * Covers README Examples 9, 10, and 11
 *
 * Exercises protected checksum paths, locked-file rehashing, timestamp
 * watching, and interactions with cleanup options for inaccessible or ignored files
 */
Return test0030(void)
{
	INITTEST;

	SLOWTEST;

	TEST(test0030_1,"Locked size change warns and keeps the protected DB row unchanged");
	TEST(test0030_2,"Watched locked timestamp drift warns and keeps the protected DB row unchanged");
	TEST(test0030_3,"Unwatched locked timestamp drift is ignored and keeps the protected DB row unchanged");
	TEST(test0030_4,"--rehash-locked synchronizes watched timestamp drift after checksum match");
	TEST(test0030_5,"--rehash-locked synchronizes timestamp drift even without --watch-timestamps");
	TEST(test0030_6,"Watched rehash reports a stored locked-checksum mismatch");
	TEST(test0030_7,"--rehash-locked reports locked content corruption and keeps the DB row unchanged");
	TEST(test0030_8,"Watched --rehash-locked reports locked content corruption and keeps the DB row unchanged");
	TEST(test0030_9,"Watched unchanged locked files complete successfully without DB drift");
	TEST(test0030_10,"Deleted locked file warns and keeps the protected DB row unchanged");
	TEST(test0030_11,"--db-drop-inaccessible does not drop an access-denied locked file");
	TEST(test0030_12,"--db-drop-inaccessible does not drop a locked file after access-check failure");
	TEST(test0030_13,"--db-drop-ignored does not drop a deleted checksum-locked file");
	TEST(test0030_14,"--rehash-locked still detects corruption in an ignored locked file");
	TEST(test0030_15,"--include does not let --db-drop-ignored drop a deleted locked file");
	TEST(test0030_16,"--rehash-locked checks ignored locked corruption outside the restored include subset");
	TEST(test0030_17,"--ignore does not let --db-drop-inaccessible drop an access-denied locked file");
	TEST(test0030_18,"--ignore does not let --db-drop-inaccessible drop a locked file after access-check failure");
	TEST(test0030_19,"Cleanup preserves an access-denied checksum-locked row despite --db-drop-inaccessible");
	TEST(test0030_20,"Cleanup preserves a checksum-locked row after access-check failure");
	TEST(test0030_21,"Cleanup preserves an ignored checksum-locked row after access-check failure");

	RETURN_STATUS;
}
