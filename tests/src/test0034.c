#include "sute.h"

/**
 * @brief Verify that lsize and asize flags are reported independently from on-disk file changes
 *
 */
static Return test0034_1(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);

	const char *fixture_path = "tests/fixtures/diffs/diff1";
	const char *tracked_file_in_source_fixture = "tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt";

	off_t sparse_file_size = 0;
	blkcnt_t sparse_blocks = 0;

	// First pass runs in TESTING mode without verbose output
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Prepare a mutable working copy while keeping a pristine backup hidden from traversal
	ASSERT(SUCCESS == prepare_mutable_fixture(fixture_path));

	ASSERT(SUCCESS == runit("--database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Case 1: grow logical size as sparse extension and preserve allocated blocks
	ASSERT(SUCCESS == make_sparse_size_change_without_allocated_block_growth(
		tracked_file_in_source_fixture,
		&sparse_file_size,
		&sparse_blocks));

	// Second and third passes run in non-TESTING mode
	// Second pass uses --verbose, third pass uses --watch-timestamps
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	ASSERT(SUCCESS == runit("--verbose --update --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0034_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(NULL != strstr(getcstring(result),"changed: lsize"));
	ASSERT(NULL == strstr(getcstring(result),"changed: asize"));

	// Case 2: rewrite file densely with same logical size so only allocated blocks differ
	ASSERT(SUCCESS == rewrite_file_dense_with_same_size(
		tracked_file_in_source_fixture,
		sparse_file_size,
		sparse_blocks));

	ASSERT(SUCCESS == runit("--update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1",result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0034_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(NULL == strstr(getcstring(result),"changed: lsize"));
	ASSERT(NULL != strstr(getcstring(result),"changed: asize"));
	ASSERT(NULL != strstr(getcstring(result),"update & rehash"));
	ASSERT(NULL != strstr(getcstring(result),"path1/AAA/BCB/CCC/a.txt"));

	ASSERT(SUCCESS == delete_path("0034_lsize_vs_asize_flags.db"));
	ASSERT(SUCCESS == restore_mutable_fixture(fixture_path));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

Return test0034(void)
{
	INITTEST;

	TEST(test0034_1,"Metadata diff flags: lsize-only and asize-only cases…");

	RETURN_STATUS;
}
