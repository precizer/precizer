#include "sute.h"
#include "mocks.h"
#include <errno.h>
#include <unistd.h>

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

	if(SUCCESS == status)
	{
		status = get_file_content(template_file,pattern);
	}

	if(SUCCESS == status)
	{
		status = m_copy(stderr_snapshot,STDERR);
	}

	if(SUCCESS == status)
	{
		status = match_pattern(stderr_snapshot,pattern,template_file);
	}

	if(SUCCESS == status)
	{
		call(m_del(STDERR));
	}

	call(m_del(stderr_snapshot));
	call(m_del(pattern));

	deliver(status);
}

/**
 * @brief Save the current TMPDIR value and clear the output pointer when unset
 *
 * @param[out] saved_tmpdir_out Heap-allocated TMPDIR copy or NULL when unset
 * @return Return status code
 */
static Return save_tmpdir_value(char **saved_tmpdir_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *original_tmpdir = NULL;

	if(saved_tmpdir_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		*saved_tmpdir_out = NULL;
		original_tmpdir = getenv("TMPDIR");

		IF(original_tmpdir != NULL)
		{
			*saved_tmpdir_out = strdup(original_tmpdir);

			if(*saved_tmpdir_out == NULL)
			{
				status = FAILURE;
			}
		}
	}

	deliver(status);
}

/**
 * @brief Restore TMPDIR to a previously saved value or unset it when NULL
 *
 * @param[in] saved_tmpdir Saved TMPDIR value or NULL when TMPDIR was unset
 * @return Return status code
 */
static Return restore_tmpdir_value(const char *saved_tmpdir)
{
	int restore_tmpdir_status = 0;

	IF(saved_tmpdir != NULL)
	{
		restore_tmpdir_status = setenv("TMPDIR",saved_tmpdir,1);
	} else {
		restore_tmpdir_status = unsetenv("TMPDIR");
	}

	return(restore_tmpdir_status == 0 ? SUCCESS : FAILURE);
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

/**
 *
 * @brief delete_path() should report remove() failure for a regular file
 *
 */
static Return test0037_5(void)
{
	INITTEST;
	SKIP_ON_EVIL_EMPIRE_OS;

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
	SKIP_ON_EVIL_EMPIRE_OS;

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
	ASSERT(SUCCESS == assert_stderr_matches("templates/0037_006.txt"));
	remove_calls = mocks_remove_call_count();
	mocks_remove_reset();
	ASSERT(remove_calls == 1U);

	ASSERT(SUCCESS == delete_path("0037_locked_tree"));

	RETURN_STATUS;
}

/**
 *
 * @brief create_directory() should accept a symlinked directory component in TMPDIR
 *
 */
static Return test0037_7(void)
{
	INITTEST;

	char *saved_tmpdir = NULL;
	bool file_exists = false;
	m_create(char,symlinked_tmpdir,MEMORY_STRING);
	m_create(char,expected_directory_path,MEMORY_STRING);
	Return restore_status = SUCCESS;
	Return cleanup_status = SUCCESS;

	ASSERT(SUCCESS == save_tmpdir_value(&saved_tmpdir));
	ASSERT(SUCCESS == create_directory("0037_symlink_parent"));
	ASSERT(SUCCESS == create_directory("0037_symlink_parent/real_tmp_root"));
	ASSERT(SUCCESS == create_symlink("real_tmp_root","0037_symlink_parent/link_tmp_root"));
	ASSERT(SUCCESS == construct_path("0037_symlink_parent/link_tmp_root",symlinked_tmpdir));
	ASSERT(SUCCESS == set_environment_variable("TMPDIR",m_text(symlinked_tmpdir)));
	ASSERT(SUCCESS == create_directory("a/b"));

	restore_status = restore_tmpdir_value(saved_tmpdir);

	if(SUCCESS == restore_status)
	{
		if(SUCCESS == construct_path("0037_symlink_parent/real_tmp_root/a/b",expected_directory_path)
		        && SUCCESS == check_file_exists(&file_exists,m_text(expected_directory_path)))
		{
			if(file_exists != true)
			{
				if(SUCCESS == status)
				{
					status = FAILURE;
				}
			}
		} else if(SUCCESS == status){
			status = FAILURE;
		}

		cleanup_status = delete_path("0037_symlink_parent");
	}

	if(SUCCESS == status)
	{
		ASSERT(SUCCESS == restore_status);
		ASSERT(SUCCESS == cleanup_status);
	}

	free(saved_tmpdir);
	call(m_del(expected_directory_path));
	call(m_del(symlinked_tmpdir));

	RETURN_STATUS;
}

/**
 *
 * @brief create_directory() should reject a TMPDIR component that resolves to a file
 *
 */
static Return test0037_8(void)
{
	INITTEST;

	char *saved_tmpdir = NULL;
	m_create(char,symlinked_tmpdir,MEMORY_STRING);
	Return restore_status = SUCCESS;
	Return cleanup_status = SUCCESS;

	ASSERT(SUCCESS == save_tmpdir_value(&saved_tmpdir));
	ASSERT(SUCCESS == create_directory("0037_symlink_bad_parent"));
	ASSERT(SUCCESS == truncate_file_to_zero_size("0037_symlink_bad_parent/file_target"));
	ASSERT(SUCCESS == create_symlink("file_target","0037_symlink_bad_parent/link_file_root"));
	ASSERT(SUCCESS == construct_path("0037_symlink_bad_parent/link_file_root",symlinked_tmpdir));
	ASSERT(SUCCESS == set_environment_variable("TMPDIR",m_text(symlinked_tmpdir)));
	ASSERT(FAILURE == create_directory("a/b"));

	restore_status = restore_tmpdir_value(saved_tmpdir);

	if(SUCCESS == restore_status)
	{
		cleanup_status = delete_path("0037_symlink_bad_parent");
	}

	if(SUCCESS == status)
	{
		ASSERT(SUCCESS == restore_status);
		ASSERT(SUCCESS == cleanup_status);
	}

	free(saved_tmpdir);
	call(m_del(symlinked_tmpdir));

	RETURN_STATUS;
}

Return test0037(void)
{
	INITTEST;

	TEST(test0037_1,"delete_path() reports NULL input path");
	TEST(test0037_2,"delete_path() reports missing path lstat() failure");
	TEST(test0037_5,"delete_path() reports remove() failure for a regular file");
	TEST(test0037_6,"delete_path() reports nftw() callback remove() failure");
	TEST(test0037_7,"create_directory() accepts a symlinked directory in TMPDIR");
	TEST(test0037_8,"create_directory() rejects a symlinked file in TMPDIR");

	RETURN_STATUS;
}
