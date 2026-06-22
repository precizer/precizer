#include "sute.h"

#define READ_FAIL_REL_PATH "tests/fixtures/diffs/diff1/path1/AAA/ZAW/D/e/f/b_file.txt"

/**
 * Simulate fread failure for a specific file to verify error handling.
 */
Return test0031(void)
{
	INITTEST;

	m_create(char,result,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,error_buffer,MEMORY_STRING);

	ASSERT(SUCCESS == set_environment_variable("TESTING","true"));

	ASSERT(SUCCESS == prepare_mutable_fixture("tests/fixtures/diffs/diff1"));

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
	ASSERT(SUCCESS == restore_mutable_fixture("tests/fixtures/diffs/diff1"));

	/* The wrapper should have injected exactly one read failure. */
	ASSERT(mocks_fread_call_count() == 1);
	mocks_fread_reset();

	m_del(pattern);
	m_del(result);
	m_del(error_buffer);

	RETURN_STATUS;
}
