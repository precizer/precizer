#include "sute.h"

/**
 * @brief Verify that lsize and asize flags are reported independently from on-disk file changes
 *
 */
static Return test0034_1(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);
	m_create(char,tracked_file_path,MEMORY_STRING);

	const char *db_filename = "0034_lsize_vs_asize_flags.db";
	const char *db_backup_filename = "0034_001.db";
	const char *relative_path = "path1/AAA/BCB/CCC/a.txt";
	const char *fixture_path = "tests/fixtures/diffs/diff1";
	const char *tracked_file_in_source_fixture = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	off_t sparse_file_size = 0;
	blkcnt_t sparse_blocks = 0;

	// First pass runs in TESTING mode without verbose output
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Prepare a mutable working copy while keeping a pristine backup hidden from traversal
	ASSERT(SUCCESS == prepare_mutable_fixture(fixture_path));

	ASSERT(SUCCESS == runit("--database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",NULL,NULL,COMPLETED,ALLOW_BOTH));

	ASSERT(SUCCESS == construct_path(tracked_file_in_source_fixture,tracked_file_path));
	CmpctStat stored_stat = {0};
	struct stat current_stat = {0};

	// Case 1: grow logical size as sparse extension and preserve allocated blocks
	ASSERT(SUCCESS == make_sparse_size_change_without_allocated_block_growth(
		tracked_file_in_source_fixture,
		&sparse_file_size,
		&sparse_blocks));

	// Both update operations run without TESTING diagnostics
	// Logical-size checks use --verbose; allocation checks also cover brief output
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	/* Restore the same database before each format and verify the stored result */
	ASSERT(SUCCESS == copy_path(db_filename,db_backup_filename));
	{
		const struct argument_test_case output_cases[] = {
			{"--check-level=QUICK --verbose --update --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_1.txt",STDERR_MUST_BE_EMPTY},
			{"--output=compact --check-level=QUICK --verbose --update --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_1_2.txt",STDERR_MUST_BE_EMPTY},
			{"--output=machine --check-level=QUICK --verbose --update --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_1_3.txt",STDERR_MUST_BE_EMPTY},
			{0}
		};

		for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
		{
			ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
			ASSERT(SUCCESS == runit(output_cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));
			ASSERT(error_buffer->length == 0);

			const char *filename = output_cases[i].stdout_template;
			ASSERT(SUCCESS == get_file_content(filename,pattern));
			ASSERT(SUCCESS == match_pattern(result,pattern,filename));

			ASSERT(NULL != strstr(m_text(result),"changed: lsize"));
			ASSERT(NULL == strstr(m_text(result),"changed: asize"));

			/* Check metadata, the independent SHA512 oracle, and completed resume state */
			ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_filename,relative_path,&stored_stat));
			ASSERT(SUCCESS == get_file_stat(m_text(tracked_file_path),&current_stat));
			ASSERT(stored_stat.st_size == sparse_file_size);
			ASSERT(stored_stat.st_blocks == current_stat.st_blocks);
			ASSERT(cmpctstat_matches_stat_timestamps(&stored_stat,&current_stat));
			ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,relative_path,m_text(tracked_file_path)));
			ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,relative_path));
			ASSERT(stored_stat.st_blocks == sparse_blocks);
		}
	}

	// Case 2: rewrite file densely with same logical size so only allocated blocks differ
	ASSERT(SUCCESS == rewrite_file_dense_with_same_size(
		tracked_file_in_source_fixture,
		sparse_file_size,
		sparse_blocks));

	/* Restore the same database before each format and verify the stored result */
	ASSERT(SUCCESS == copy_path(db_filename,db_backup_filename));
	{
		const struct argument_test_case output_cases[] = {
			{"--check-level=QUICK --update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_2.txt",STDERR_MUST_BE_EMPTY},
			{"--output=compact --check-level=QUICK --update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_2_2.txt",STDERR_MUST_BE_EMPTY},
			{"--output=machine --check-level=QUICK --update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_2_3.txt",STDERR_MUST_BE_EMPTY},
			{"--output=machine --verbose --check-level=QUICK --update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",COMPLETED,"templates/0034_001_2_4.txt",STDERR_MUST_BE_EMPTY},
			{0}
		};

		for(size_t i = 0; output_cases[i].arguments != NULL && SUCCESS == status; ++i)
		{
			ASSERT(SUCCESS == copy_path(db_backup_filename,db_filename));
			ASSERT(SUCCESS == runit(output_cases[i].arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));
			ASSERT(error_buffer->length == 0);

			const char *filename = output_cases[i].stdout_template;
			ASSERT(SUCCESS == get_file_content(filename,pattern));
			ASSERT(SUCCESS == match_pattern(result,pattern,filename));

			/* Full and verbose output expose allocation changes; brief output omits them */
			if(i == 0 || i == 3)
			{
				ASSERT(NULL == strstr(m_text(result),"changed: lsize"));
				ASSERT(NULL != strstr(m_text(result),"changed: asize"));
				ASSERT(NULL != strstr(m_text(result),"rehashed"));
				ASSERT(NULL != strstr(m_text(result),"path1/AAA/BCB/CCC/a.txt"));
			}

			/* Check metadata, the independent SHA512 oracle, and completed resume state */
			ASSERT(SUCCESS == db_read_cmpctstat_by_relative_path(db_filename,relative_path,&stored_stat));
			ASSERT(SUCCESS == get_file_stat(m_text(tracked_file_path),&current_stat));
			ASSERT(stored_stat.st_size == sparse_file_size);
			ASSERT(stored_stat.st_blocks == current_stat.st_blocks);
			ASSERT(cmpctstat_matches_stat_timestamps(&stored_stat,&current_stat));
			ASSERT(SUCCESS == db_final_sha512_matches_file(db_filename,relative_path,m_text(tracked_file_path)));
			ASSERT(SUCCESS == db_resume_state_is_empty(db_filename,relative_path));
			ASSERT(stored_stat.st_blocks != sparse_blocks);
		}
	}

	ASSERT(SUCCESS == delete_path("0034_lsize_vs_asize_flags.db"));
	ASSERT(SUCCESS == restore_mutable_fixture(fixture_path));

	call(delete_path_if_present(db_backup_filename));
	m_del(tracked_file_path);
	m_del(error_buffer);
	m_del(pattern);
	m_del(result);

	RETURN_STATUS;
}

/**
 * @brief Run metadata-difference reporting coverage for logical and allocated size changes
 *
 * @return Return status code
 */
Return test0034(void)
{
	INITTEST;

	TEST(test0034_1,"Metadata diff flags: lsize-only and asize-only cases");

	RETURN_STATUS;
}
