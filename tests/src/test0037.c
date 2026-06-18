#include "sute.h"

static Return assert_stderr_matches(const char *template_file)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,pattern,MEMORY_STRING);
	m_create(char,stderr_snapshot,MEMORY_STRING);

	if(template_file == NULL)
	{
		status = FAILURE;
	}

	run(get_file_content(template_file,pattern));
	run(m_copy(stderr_snapshot,STDERR));
	run(match_pattern(stderr_snapshot,pattern,template_file));

	run(m_del(STDERR));

	call(m_del(stderr_snapshot));
	call(m_del(pattern));

	deliver(status);
}

/**
 *
 * @brief delete_path() should report a NULL input path
 *
 */
static Return test0037_1(void)
{
	INITTEST;

	call(m_del(STDERR));

	ASSERT(FAILURE == delete_path(NULL));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_001.txt"));

	RETURN_STATUS;
}

/**
 *
 * @brief delete_path() should report lstat() failure for a missing path
 *
 */
static Return test0037_2(void)
{
	INITTEST;

	call(m_del(STDERR));

	ASSERT(FAILURE == delete_path("delete_path_missing.txt"));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_002.txt"));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 *
 * @brief delete_path() should report remove() failure for a regular file
 *
 */
static Return test0037_3(void)
{
	INITTEST;

	size_t remove_calls = 0;
	const char *file_path = "0037_read_only_dir/file.txt";

	ASSERT(SUCCESS == create_directory("0037_read_only_dir"));
	ASSERT(SUCCESS == truncate_file_to_zero_size(file_path));

	call(m_del(STDERR));
	mocks_remove_reset();
	mocks_remove_set_target_suffix(file_path);
	mocks_remove_set_errno(EACCES);
	mocks_remove_enable(true);

	ASSERT(FAILURE == delete_path(file_path));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_003.txt"));
	remove_calls = mocks_remove_call_count();
	mocks_remove_reset();
	ASSERT(remove_calls == 1U);

	ASSERT(SUCCESS == delete_path("0037_read_only_dir"));

	RETURN_STATUS;
}

/**
 *
 * @brief delete_path() should report callback remove() failure during nftw()
 *
 */
static Return test0037_4(void)
{
	INITTEST;

	size_t remove_calls = 0;
	const char *file_path = "0037_locked_tree/a/b/file.txt";

	ASSERT(SUCCESS == create_directory("0037_locked_tree/a/b"));
	ASSERT(SUCCESS == truncate_file_to_zero_size(file_path));

	call(m_del(STDERR));
	mocks_remove_reset();
	mocks_remove_set_target_suffix(file_path);
	mocks_remove_set_errno(EACCES);
	mocks_remove_enable(true);

	ASSERT(FAILURE == delete_path("0037_locked_tree"));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_004.txt"));
	remove_calls = mocks_remove_call_count();
	mocks_remove_reset();
	ASSERT(remove_calls == 1U);

	ASSERT(SUCCESS == delete_path("0037_locked_tree"));

	RETURN_STATUS;
}
#endif

/**
 * @brief Run delete_path() diagnostics unit tests
 */
Return test0037(void)
{
	INITTEST;

	TEST(test0037_1,"delete_path() reports NULL input path");
	TEST(test0037_2,"delete_path() reports missing path lstat() failure");
#ifndef EVIL_EMPIRE_OS
	TEST(test0037_3,"delete_path() reports remove() failure for a regular file");
	TEST(test0037_4,"delete_path() reports nftw() callback remove() failure");
#endif

	RETURN_STATUS;
}
