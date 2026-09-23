#include "sute.h"

/**
 * @brief Verify updates and DB comparisons after content, deletion, and permission changes
 *
 * Content growth triggers rehashing with or without --watch-timestamps.
 * A permission-only change updates metadata without this option and rehashes
 * unchanged content when it is enabled.
 * Both database comparisons report the deleted file and the content change.
 * Full output is checked with TESTING enabled and disabled
 *
 * @return Return status code
 */
static Return test0016_1(void)
{
	INITTEST;

	m_create(char,pattern,MEMORY_STRING);

	// Create memory for the result
	m_create(char,result,MEMORY_STRING);
	m_create(char,chunk,MEMORY_STRING);

	m_create(char,target_path,MEMORY_STRING);
	struct stat stat_before = {0};
	struct stat stat_after = {0};

	// Preparation for the test
	const char *diff1_fixture_path = "tests/fixtures/diffs/diff1";
	const char *permission_file_path = "tests/fixtures/diffs/diff1/2/AAA/BBB/CZC/a.txt";
	ASSERT(SUCCESS == prepare_mutable_fixture(diff1_fixture_path));

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == runit("--database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_copy(result,chunk));

	ASSERT(SUCCESS == copy_path("database1.db","database2.db"));

	ASSERT(SUCCESS == add_string_to("PWOEUNVSODNLKUHGE","tests/fixtures/diffs/diff1/1/AAA/BCB/CCC/a.txt"));

	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1/path2/AAA/ZAW/D/e/f/b_file.txt"));

	// Toggle owner write permission to change ctime while preserving content, mtime, and read access
	ASSERT(SUCCESS == construct_path(permission_file_path,target_path));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_before));
	unsigned int changed_mode = ((unsigned int)stat_before.st_mode & 07777U) ^ S_IWUSR;
	ASSERT(SUCCESS == change_mode(permission_file_path,changed_mode));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_after));
	ASSERT(stat_after.st_mode == (stat_before.st_mode ^ S_IWUSR));
	ASSERT(stat_before.st_ctim.tv_sec != stat_after.st_ctim.tv_sec
		|| stat_before.st_ctim.tv_nsec != stat_after.st_ctim.tv_nsec);
	ASSERT(stat_before.st_mtim.tv_sec == stat_after.st_mtim.tv_sec
		&& stat_before.st_mtim.tv_nsec == stat_after.st_mtim.tv_nsec);
	ASSERT(stat_before.st_size == stat_after.st_size);
	ASSERT(stat_before.st_blocks == stat_after.st_blocks);

	// Update without --watch-timestamps: rehash changed content, update ctime-only
	// metadata without rehashing, and remove the deleted file's DB record
	ASSERT(SUCCESS == runit("--update --check-level=QUICK --database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	// Compare against the original DB: report the deleted file and the changed checksum
	const char *compare_arguments = "--compare database1.db database2.db";
	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	// Restore the original DB so the next update processes the same changes
	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	// Update with --watch-timestamps: rehash the file whose ctime changed as well
	const char *watch_update_arguments = "--watch-timestamps --update --database=database1.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(watch_update_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	// Verify that rehashing the permission-only change adds no checksum difference
	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	// Check the complete combined output for creation, both updates, and both comparisons
	const char *filename = "templates/0016_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);
	m_del(chunk);

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	ASSERT(SUCCESS == runit("--update --database=database1.db tests/fixtures/diffs/diff1",chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_copy(result,chunk));

	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	ASSERT(SUCCESS == copy_path("database2.db","database1.db"));

	ASSERT(SUCCESS == runit(watch_update_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	ASSERT(SUCCESS == runit(compare_arguments,chunk,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == m_concat_strings(result,chunk));

	filename = "templates/0016_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Clean to use it iteratively
	m_del(pattern);
	m_del(result);
	m_del(chunk);

	// Clean up test results
	ASSERT(SUCCESS == delete_path("database1.db"));
	ASSERT(SUCCESS == delete_path("database2.db"));
	ASSERT(SUCCESS == restore_mutable_fixture(diff1_fixture_path));
	m_del(target_path);

	RETURN_STATUS;
}

/**
 * @brief Rehash same-size content changes when mtime changes without --watch-timestamps
 *
 * @return Return status code
 */
static Return test0016_2(void)
{
	INITTEST;

	const char *fixture_path = "0016_mtime";
	const char *file_path = "0016_mtime/file.txt";
	const char *db_filename = "0016_mtime.db";
	m_create(char,target_path,MEMORY_STRING);
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	struct stat stat_before = {0};
	struct stat stat_after = {0};
	CmpctStat saved_stat = {0};
	sqlite3_int64 offset = -1;
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};

	// Create a three-byte file containing "aaa" and store its checksum in the DB
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == create_directory(fixture_path));
	ASSERT(SUCCESS == replase_to_string("aaa",file_path));
	ASSERT(SUCCESS == construct_path(file_path,target_path));
	ASSERT(SUCCESS == runit("--database=0016_mtime.db 0016_mtime",NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Save the completed checksum and verify it independently before changing the file
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_before));
	ASSERT(offset == 0);
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,"file.txt",m_text(target_path)));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_before));

	// Use the saved mtime as a deterministic reference and preserve atime
	struct timespec file_times[2] = {{0}};
	file_times[0].tv_nsec = UTIME_OMIT;
	file_times[1] = stat_before.st_mtim;
	file_times[1].tv_sec++;

	// Replace "aaa" with "bbb" and force mtime one second past its original value
	ASSERT(SUCCESS == replase_to_string("bbb",file_path));
	ASSERT(utimensat(0,m_text(target_path),file_times,0) == 0);

	// Confirm that the file is still three bytes long and its mtime has changed
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_after));
	ASSERT(stat_before.st_size == 3 && stat_after.st_size == 3);
	ASSERT(stat_before.st_mtim.tv_sec != stat_after.st_mtim.tv_sec
		|| stat_before.st_mtim.tv_nsec != stat_after.st_mtim.tv_nsec);

	// Update without --watch-timestamps to exercise default mtime detection
	ASSERT(SUCCESS == runit("--update --database=0016_mtime.db 0016_mtime",result,NULL,COMPLETED,ALLOW_BOTH));

	// Check the full output for a rehash of file.txt and exactly three hashed bytes
	const char *filename = "templates/0016_002_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// Verify that the new checksum is complete and differs from the checksum of "aaa"
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_after));
	ASSERT(offset == 0);
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) != 0);

	// Independently hash "bbb" with Monocypher and compare it with the stored checksum
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,"file.txt",m_text(target_path)));

	// Verify that the DB contains the file's updated mtime and ctime
	ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_filename,"file.txt",&saved_stat));
	ASSERT(cmpctstat_matches_stat_timestamps(&saved_stat,&stat_after));

	// Remove the temporary DB and files, then release the test buffers
	call(delete_path_if_present(db_filename));
	call(delete_path_if_present(fixture_path));
	m_del(pattern);
	m_del(result);
	m_del(target_path);

	RETURN_STATUS;
}

/**
 * @brief Update metadata without reading file content after a permission change without --watch-timestamps
 *
 * @return Return status code
 */
static Return test0016_3(void)
{
	INITTEST;

	const char *fixture_path = "0016_chmod";
	const char *file_path = "0016_chmod/file.txt";
	const char *db_filename = "0016_chmod.db";
	m_create(char,target_path,MEMORY_STRING);
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	struct stat stat_before = {0};
	struct stat stat_after = {0};
	CmpctStat saved_stat = {0};
	sqlite3_int64 offset = -1;
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == create_directory(fixture_path));
	ASSERT(SUCCESS == replase_to_string("aaa",file_path));
	ASSERT(SUCCESS == change_mode(file_path,0600));
	ASSERT(SUCCESS == construct_path(file_path,target_path));
	ASSERT(SUCCESS == runit("--database=0016_chmod.db 0016_chmod",NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_before));
	ASSERT(offset == 0);
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_before));

	// Keep the file readable while changing ctime without modifying content or mtime
	ASSERT(SUCCESS == change_mode(file_path,0644));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_after));
	ASSERT(stat_before.st_mode != stat_after.st_mode);
	ASSERT(stat_before.st_ctim.tv_sec != stat_after.st_ctim.tv_sec
		|| stat_before.st_ctim.tv_nsec != stat_after.st_ctim.tv_nsec);
	ASSERT(stat_before.st_mtim.tv_sec == stat_after.st_mtim.tv_sec
		&& stat_before.st_mtim.tv_nsec == stat_after.st_mtim.tv_nsec);
	ASSERT(stat_before.st_size == stat_after.st_size);
	ASSERT(stat_before.st_blocks == stat_after.st_blocks);

	// Update without --watch-timestamps and check the full output for
	// a metadata-only update with zero bytes hashed by the application
	ASSERT(SUCCESS == runit("--update --database=0016_chmod.db 0016_chmod",result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0016_002_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	// A zero offset denotes a completed checksum, which must retain its original value
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_after));
	ASSERT(offset == 0);
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);

	// Independently hash the file in the test to verify the checksum stored in the DB
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,"file.txt",m_text(target_path)));

	// Confirm that the DB saved the changed ctime and preserved the file's mtime
	ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_filename,"file.txt",&saved_stat));
	ASSERT(cmpctstat_matches_stat_timestamps(&saved_stat,&stat_after));

	call(delete_path_if_present(db_filename));
	call(delete_path_if_present(fixture_path));
	m_del(pattern);
	m_del(result);
	m_del(target_path);

	RETURN_STATUS;
}

/**
 * @brief Rehash unchanged content after a permission change when --watch-timestamps watches ctime
 *
 * @return Return status code
 */
static Return test0016_4(void)
{
	INITTEST;

	const char *fixture_path = "0016_watch_chmod";
	const char *file_path = "0016_watch_chmod/file.txt";
	const char *db_filename = "0016_watch_chmod.db";
	m_create(char,target_path,MEMORY_STRING);
	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	struct stat stat_before = {0};
	struct stat stat_after = {0};
	CmpctStat saved_stat = {0};
	sqlite3_int64 offset = -1;
	unsigned char checksum_before[SHA512_DIGEST_LENGTH] = {0};
	unsigned char checksum_after[SHA512_DIGEST_LENGTH] = {0};

	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));
	ASSERT(SUCCESS == create_directory(fixture_path));
	ASSERT(SUCCESS == replase_to_string("aaa",file_path));
	ASSERT(SUCCESS == change_mode(file_path,0644));
	ASSERT(SUCCESS == construct_path(file_path,target_path));
	ASSERT(SUCCESS == runit("--database=0016_watch_chmod.db 0016_watch_chmod",NULL,NULL,COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_before));
	ASSERT(offset == 0);
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_before));

	// A fresh permission change supplies ctime drift for the watched update
	ASSERT(SUCCESS == change_mode(file_path,0600));
	ASSERT(SUCCESS == get_file_stat(m_text(target_path),&stat_after));
	ASSERT(stat_before.st_mode != stat_after.st_mode);
	ASSERT(stat_before.st_ctim.tv_sec != stat_after.st_ctim.tv_sec
		|| stat_before.st_ctim.tv_nsec != stat_after.st_ctim.tv_nsec);
	ASSERT(stat_before.st_mtim.tv_sec == stat_after.st_mtim.tv_sec
		&& stat_before.st_mtim.tv_nsec == stat_after.st_mtim.tv_nsec);
	ASSERT(stat_before.st_size == stat_after.st_size);
	ASSERT(stat_before.st_blocks == stat_after.st_blocks);

	ASSERT(SUCCESS == runit("--update --watch-timestamps --database=0016_watch_chmod.db 0016_watch_chmod",result,NULL,COMPLETED,ALLOW_BOTH));
	const char *filename = "templates/0016_002_3.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));
	ASSERT(SUCCESS == read_final_sha512_from_db(db_filename,"file.txt",&offset,checksum_after));
	ASSERT(offset == 0);
	ASSERT(memcmp(checksum_before,checksum_after,sizeof(checksum_before)) == 0);
	ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,"file.txt",m_text(target_path)));
	ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_filename,"file.txt",&saved_stat));
	ASSERT(cmpctstat_matches_stat_timestamps(&saved_stat,&stat_after));

	call(delete_path_if_present(db_filename));
	call(delete_path_if_present(fixture_path));
	m_del(pattern);
	m_del(result);
	m_del(target_path);

	RETURN_STATUS;
}

/**
 * @brief Verify default and optional timestamp-driven checksum updates
 *
 * @return Return status code
 */
Return test0016(void)
{
	INITTEST;

	TEST(test0016_1,"Content, deletion, and permission updates with and without --watch-timestamps");
	TEST(test0016_2,"A changed mtime rehashes same-size content without --watch-timestamps");
	TEST(test0016_3,"Permission changes update metadata without hashing by default");
	TEST(test0016_4,"Permission changes rehash content when --watch-timestamps watches ctime");

	RETURN_STATUS;
}
