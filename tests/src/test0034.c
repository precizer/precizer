#include "sute.h"

/**
 * @brief Make a file sparse by extending logical size while keeping allocated blocks unchanged
 *
 */
static Return make_sparse_size_change_without_allocated_block_growth(
	const char *relative_path_to_tmpdir,
	off_t      *new_size_out,
	blkcnt_t   *blocks_after_change_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat before_stat = {0};
	struct stat after_stat = {0};
	create(char,absolute_path);

	if(relative_path_to_tmpdir == NULL || new_size_out == NULL || blocks_after_change_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(getcstring(absolute_path),&before_stat);
	}

	if(SUCCESS == status)
	{
		const off_t grown_size = before_stat.st_size + (off_t)131072;

		// Grow logical size via truncate to create a sparse tail without writing payload bytes
		if(grown_size <= before_stat.st_size)
		{
			status = FAILURE;
		} else if(truncate(getcstring(absolute_path),grown_size) != 0){
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(getcstring(absolute_path),&after_stat);
	}

	if(SUCCESS == status && after_stat.st_size <= before_stat.st_size)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && after_stat.st_blocks != before_stat.st_blocks)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		*new_size_out = after_stat.st_size;
		*blocks_after_change_out = after_stat.st_blocks;
	}

	del(absolute_path);

	return(status);
}

/**
 * @brief Rewrite file content with dense bytes while preserving logical size
 *
 */
static Return rewrite_file_dense_with_same_size(
	const char   *relative_path_to_tmpdir,
	const off_t  target_size,
	const blkcnt_t blocks_before_rewrite)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	struct stat after_stat = {0};
	unsigned char buffer[4096];
	create(char,absolute_path);

	memset(buffer,'X',sizeof(buffer));

	if(relative_path_to_tmpdir == NULL || target_size <= 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		// Rewrite the whole file with real bytes while keeping the same logical size
		file = fopen(getcstring(absolute_path),"wb");

		if(file == NULL)
		{
			status = FAILURE;
		}
	}

	off_t written = 0;

	while(SUCCESS == status && written < target_size)
	{
		const off_t remaining = target_size - written;
		size_t chunk = sizeof(buffer);

		if(remaining < (off_t)chunk)
		{
			chunk = (size_t)remaining;
		}

		if(fwrite(buffer,sizeof(unsigned char),chunk,file) != chunk)
		{
			status = FAILURE;
		} else {
			written += (off_t)chunk;
		}
	}

	if(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(getcstring(absolute_path),&after_stat);
	}

	if(SUCCESS == status && after_stat.st_size != target_size)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && after_stat.st_blocks == blocks_before_rewrite)
	{
		status = FAILURE;
	}

	del(absolute_path);

	return(status);
}

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
