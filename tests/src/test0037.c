#include "sute.h"
#include "mocks.h"
#include <errno.h>
#include <unistd.h>

static Return assert_stderr_matches(
	const char *template_file)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	create(char,pattern);
	create(char,stderr_snapshot);

	if(template_file == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = get_file_content(template_file,pattern);
	}

	if(SUCCESS == status)
	{
		status = copy(stderr_snapshot,STDERR);
	}

	if(SUCCESS == status)
	{
		status = match_pattern(stderr_snapshot,pattern,template_file);
	}

	if(SUCCESS == status)
	{
		call(del(STDERR));
	}

	call(del(stderr_snapshot));
	call(del(pattern));

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

	call(del(STDERR));

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

	call(del(STDERR));

	ASSERT(FAILURE == delete_path("delete_path_missing.txt"));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_002.txt"));

	RETURN_STATUS;
}

/**
 *
 * @brief construct_path() should report NULL input arguments
 *
 */
static Return test0037_3(void)
{
	INITTEST;

	create(char,path);

	call(del(STDERR));

	ASSERT(FAILURE == construct_path(NULL,path));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_003.txt"));

	call(del(path));

	RETURN_STATUS;
}

/**
 *
 * @brief construct_path() should report a missing TMPDIR
 *
 */
static Return test0037_4(void)
{
	INITTEST;

	create(char,path);
	char *saved_tmpdir = NULL;
	const char *original_tmpdir = getenv("TMPDIR");
	int restore_tmpdir_status = 0;

	if(original_tmpdir != NULL)
	{
		saved_tmpdir = strdup(original_tmpdir);
		ASSERT(saved_tmpdir != NULL);
	}

	if(SUCCESS == status)
	{
		ASSERT(unsetenv("TMPDIR") == 0);
	}

	call(del(STDERR));

	if(SUCCESS == status)
	{
		ASSERT(FAILURE == construct_path("tmpdir_missing.txt",path));
		ASSERT(SUCCESS == assert_stderr_matches("templates/0037_004.txt"));
	}

	if(saved_tmpdir != NULL)
	{
		restore_tmpdir_status = setenv("TMPDIR",saved_tmpdir,1);
	} else {
		restore_tmpdir_status = unsetenv("TMPDIR");
	}

	if(SUCCESS == status)
	{
		ASSERT(restore_tmpdir_status == 0);
	}

	free(saved_tmpdir);
	call(del(path));

	RETURN_STATUS;
}

/**
 *
 * @brief delete_path() should report remove() failure for a regular file
 *
 */
static Return test0037_5(void)
{
	INITTEST;

	size_t remove_calls = 0;
	const char *file_path = "0037_read_only_dir/file.txt";

#ifdef EVIL_EMPIRE_OS
	deliver(DONOTHING);
#endif

	ASSERT(SUCCESS == create_directory("0037_read_only_dir"));
	ASSERT(SUCCESS == truncate_file_to_zero_size(file_path));

	call(del(STDERR));
	mocks_remove_reset();
	mocks_remove_set_target_suffix(file_path);
	mocks_remove_set_errno(EACCES);
	mocks_remove_enable(true);

	ASSERT(FAILURE == delete_path(file_path));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_005.txt"));
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
static Return test0037_6(void)
{
	INITTEST;

	size_t remove_calls = 0;
	const char *file_path = "0037_locked_tree/a/b/file.txt";

#ifdef EVIL_EMPIRE_OS
	deliver(DONOTHING);
#endif

	ASSERT(SUCCESS == create_directory("0037_locked_tree/a/b"));
	ASSERT(SUCCESS == truncate_file_to_zero_size(file_path));

	call(del(STDERR));
	mocks_remove_reset();
	mocks_remove_set_target_suffix(file_path);
	mocks_remove_set_errno(EACCES);
	mocks_remove_enable(true);

	ASSERT(FAILURE == delete_path("0037_locked_tree"));
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_006.txt"));
	remove_calls = mocks_remove_call_count();
	mocks_remove_reset();
	ASSERT(remove_calls == 1U);

	ASSERT(SUCCESS == delete_path("0037_locked_tree"));

	RETURN_STATUS;
}

Return test0037(void)
{
	INITTEST;

	TEST(test0037_1,"delete_path() reports NULL input path…");
	TEST(test0037_2,"delete_path() reports missing path lstat() failure…");
	TEST(test0037_3,"construct_path() reports NULL input arguments…");
	TEST(test0037_4,"construct_path() reports missing TMPDIR…");
	TEST(test0037_5,"delete_path() reports remove() failure for a regular file…");
	TEST(test0037_6,"delete_path() reports nftw() callback remove() failure…");

	RETURN_STATUS;
}
