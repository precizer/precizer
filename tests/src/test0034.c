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

	const char *prepare_fixture_command = "cd ${TMPDIR};"
	        "rm -f 0034_lsize_vs_asize_flags.db;"
	        "rm -rf tests/fixtures/diff1_backup;"
	        "mv tests/fixtures/diffs/diff1 tests/fixtures/diff1_backup;"
	        "cp -a tests/fixtures/diff1_backup tests/fixtures/diffs/diff1;";

	const char *cleanup_fixture_command = "cd ${TMPDIR};"
	        "rm -f 0034_lsize_vs_asize_flags.db;"
	        "rm -rf tests/fixtures/diffs/diff1;"
	        "mv tests/fixtures/diff1_backup tests/fixtures/diffs/diff1;";

	off_t sparse_file_size = 0;
	blkcnt_t sparse_blocks = 0;

	// First pass runs in TESTING mode without verbose output
	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	// Prepare fixture copy and clean previous DB
	ASSERT(SUCCESS == external_call(prepare_fixture_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	const char *arguments = "--database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,NULL,NULL,COMPLETED,ALLOW_BOTH));

	// Case 1: grow logical size as sparse extension and preserve allocated blocks
	ASSERT(SUCCESS == make_sparse_size_change_without_allocated_block_growth(
		"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",
		&sparse_file_size,
		&sparse_blocks));

	// Second and third passes run in non-TESTING mode
	// Second pass uses --verbose, third pass uses --watch-timestamps
	ASSERT(SUCCESS == set_environment_variable("TESTING","false"));

	arguments = "--verbose --update --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	const char *filename = "templates/0034_001_1.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(NULL != strstr(getcstring(result),"changed: lsize"));
	ASSERT(NULL == strstr(getcstring(result),"changed: asize"));

	// Case 2: rewrite file densely with same logical size so only allocated blocks differ
	ASSERT(SUCCESS == rewrite_file_dense_with_same_size(
		"tests/fixtures/diffs/diff1/path1/AAA/BCB/CCC/a.txt",
		sparse_file_size,
		sparse_blocks));

	arguments = "--update --watch-timestamps --database=0034_lsize_vs_asize_flags.db tests/fixtures/diffs/diff1";
	ASSERT(SUCCESS == runit(arguments,result,NULL,COMPLETED,ALLOW_BOTH));

	filename = "templates/0034_001_2.txt";
	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(NULL == strstr(getcstring(result),"changed: lsize"));
	ASSERT(NULL != strstr(getcstring(result),"changed: asize"));
	ASSERT(NULL != strstr(getcstring(result),"update & rehash"));
	ASSERT(NULL != strstr(getcstring(result),"path1/AAA/BCB/CCC/a.txt"));

	ASSERT(SUCCESS == external_call(cleanup_fixture_command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	del(pattern);
	del(result);

	RETURN_STATUS;
}

Return test0034(void)
{
	INITTEST;

	TEST(test0034_1,"Metadata diff flags: lsize-only and asize-only cases...");

	RETURN_STATUS;
}
