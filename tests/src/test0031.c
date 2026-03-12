#include "sute.h"
#include "mocks.h"
#include <errno.h>

#define READ_FAIL_REL_PATH "tests/fixtures/diffs/diff1/path1/AAA/ZAW/D/e/f/b_file.txt"

/**
 * Simulate fread failure for a specific file to verify error handling.
 */
Return test0031(void)
{
	INITTEST;

	create(char,result);
	create(char,pattern);
	create(char,error_buffer);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diffs/diff1","tests/fixtures/diff1_backup"));
	ASSERT(SUCCESS == copy_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	const char *arguments = "--database=read_fail.db --progress"
	        " tests/fixtures/diffs/diff1";

	/* Configure the fread mock to fail once for the target file only. */
	mocks_fread_reset();
	mocks_fread_set_target_suffix(READ_FAIL_REL_PATH);
	mocks_fread_enable(true);
	mocks_fread_set_errno(EIO);

	ASSERT(SUCCESS == runit(arguments,result,error_buffer,COMPLETED,ALLOW_BOTH));

	/* Always disable the mock and restore the previous run mode. */
	mocks_fread_enable(false);

	ASSERT(error_buffer->length == 0);

	const char *filename = "templates/0031_001.txt";

	ASSERT(SUCCESS == get_file_content(filename,pattern));
	ASSERT(SUCCESS == match_pattern(result,pattern,filename));

	ASSERT(SUCCESS == delete_path("read_fail.db"));
	ASSERT(SUCCESS == delete_path("tests/fixtures/diffs/diff1"));

	ASSERT(SUCCESS == move_path("tests/fixtures/diff1_backup","tests/fixtures/diffs/diff1"));

	/* The wrapper should have injected exactly one read failure. */
	ASSERT(mocks_fread_call_count() == 1);
	mocks_fread_reset();

	del(pattern);
	del(result);
	del(error_buffer);

	RETURN_STATUS;
}
