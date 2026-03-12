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

	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL || relative_path == NULL))
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(db_filename,db_path);
	}

	if(SUCCESS == status && SQLITE_OK != sqlite3_open_v2(getcstring(db_path),&db,SQLITE_OPEN_READWRITE,NULL))
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
	CmpctStat *stat_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	const char *sql = "SELECT stat FROM files WHERE relative_path = ?1;";
	create(char,db_path);

	if(SUCCESS == status && (db_filename == NULL || relative_path == NULL || stat_out == NULL))
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

static bool cmpctstat_matches_stat_timestamps(
	const CmpctStat *db_stat,
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
 * @brief README --lock-checksum example: on-disk size change of a locked file
 * with --rehash-locked triggers WARNING
 */
static Return test0030_1(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s1.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_001_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == write_string_to_file("pad",
	        "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",
	        FILE_WRITE_APPEND));

	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s1.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0030_001_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s1.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief README --lock-checksum example: timestamp-only drift with
 * --watch-timestamps and without --rehash-locked triggers WARNING
 */
static Return test0030_2(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s2.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_002_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",999));

	arguments = "--update --watch-timestamps --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s2.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0030_002_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s2.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief README --lock-checksum example: timestamp-only drift without
 * --watch-timestamps and without --rehash-locked completes successfully
 */
static Return test0030_3(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s3.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_003_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",999));

	arguments = "--update --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s3.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0030_003_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s3.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief README --lock-checksum example: timestamp-only drift with
 * --watch-timestamps and with --rehash-locked completes successfully and
 * synchronizes DB timestamps
 */
static Return test0030_4(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);
	create(char,target_path);
	struct stat file_stat_before = {0};
	struct stat file_stat_after = {0};
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s4.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_004_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == construct_path("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",target_path));

	ASSERT(SUCCESS == read_cmpctstat_from_db("lock_s4.db","path1/AAA/BCB/CCC/a.txt",&db_stat_before));

	ASSERT(SUCCESS == get_file_stat(getcstring(target_path),&file_stat_before));

	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_before));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",999));

	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s4.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0030_004_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == read_cmpctstat_from_db("lock_s4.db","path1/AAA/BCB/CCC/a.txt",&db_stat_after));

	ASSERT(SUCCESS == get_file_stat(getcstring(target_path),&file_stat_after));

	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_after,&file_stat_after));

	ASSERT(SUCCESS == delete_path("lock_s4.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(target_path);
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief README --lock-checksum example: timestamp-only drift without
 * --watch-timestamps and with --rehash-locked completes successfully and
 * synchronizes DB timestamps
 */
static Return test0030_5(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);
	create(char,target_path);
	struct stat file_stat_before = {0};
	struct stat file_stat_after = {0};
	CmpctStat db_stat_before = {0};
	CmpctStat db_stat_after = {0};

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s5.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_005_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == construct_path("tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",target_path));

	ASSERT(SUCCESS == read_cmpctstat_from_db("lock_s5.db","path1/AAA/BCB/CCC/a.txt",&db_stat_before));

	ASSERT(SUCCESS == get_file_stat(getcstring(target_path),&file_stat_before));

	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_before,&file_stat_before));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",999));

	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s5.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0030_005_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == read_cmpctstat_from_db("lock_s5.db","path1/AAA/BCB/CCC/a.txt",&db_stat_after));

	ASSERT(SUCCESS == get_file_stat(getcstring(target_path),&file_stat_after));

	ASSERT(cmpctstat_matches_stat_timestamps(&db_stat_after,&file_stat_after));

	ASSERT(SUCCESS == delete_path("lock_s5.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(target_path);
	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * @brief README --lock-checksum example extension: timestamp-only drift on one
 * locked file plus a stored checksum mismatch on another locked file triggers
 * WARNING under --watch-timestamps and --rehash-locked
 */
static Return test0030_6(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s6.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_006_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Bump file mtime by a nanosecond delta without changing file content
	ASSERT(SUCCESS == touch_file_mtime_with_reference_delta_ns(NULL,"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",999));

	// Corrupt the stored checksum for a locked file without touching it on disk
	ASSERT(SUCCESS == db_tamper_locked_checksum("lock_s6.db","path1/AAA/ZAW/A/b/c/a_file.txt"));

	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s6.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0030_006_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s6.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * README --lock-checksum example extension: on-disk content change with
 * --rehash-locked should trigger WARNING
 */
static Return test0030_7(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s7.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_007_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == tamper_locked_file_bytes("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt"));

	arguments = "--update --rehash-locked --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s7.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0030_007_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s7.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * Same as test0030_7 but with --watch-timestamps; per README example, this option
 * does not change the --rehash-locked outcome
 */
static Return test0030_8(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s8.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_008_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == tamper_locked_file_bytes("tests/fixtures/diffs/diff1/path1/AAA/ZAW/A/b/c/a_file.txt"));

	arguments = "--update --watch-timestamps --rehash-locked "
	        "--lock-checksum=\"^path1/.*\" --database=lock_s8.db "
	        "tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,WARNING,ALLOW_BOTH));

	filename = "templates/0030_008_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s8.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 * README --lock-checksum example: size and timestamps match,
 * --watch-timestamps enabled, --rehash-locked disabled -> SUCCESS
 */
static Return test0030_9(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=lock_s9.db --progress "
	        "--lock-checksum=\"^path1/.*\" tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0030_009_1.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	arguments = "--update --watch-timestamps --lock-checksum=\"^path1/.*\" "
	        "--database=lock_s9.db tests/fixtures/diffs/diff1";

	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0030_009_2.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));

	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("lock_s9.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));
	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

/**
 *
 * README --lock-checksum examples.
 * Scenarios with --lock-checksum, --rehash-locked, and --watch-timestamps
 *
 */
Return test0030(void)
{
	INITTEST;

	TEST(test0030_1,"Size change with locked checksum triggers a warning…");
	TEST(test0030_2,"Timestamp drift with --watch-timestamps triggers a warning…");
	TEST(test0030_3,"Timestamp drift without --watch-timestamps completes successfully…");
	TEST(test0030_4,"Timestamp drift with --watch-timestamps and --rehash-locked completes successfully…");
	TEST(test0030_5,"Timestamp drift without --watch-timestamps and with --rehash-locked completes successfully…");
	TEST(test0030_6,"Locked checksum mismatch in DB triggers a warning…");
	TEST(test0030_7,"Locked file content change triggers a warning…");
	TEST(test0030_8,"Locked file content change with --watch-timestamps triggers a warning…");
	TEST(test0030_9,"No changes with --watch-timestamps completes successfully…");

	RETURN_STATUS;
}
