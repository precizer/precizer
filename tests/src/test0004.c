#include "sute.h"
#include <stdlib.h>
#include <string.h>

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
 * @brief create_directory() should accept a symlinked directory component in TMPDIR
 *
 */
static Return test0004_1(void)
{
	INITTEST;

	char *saved_tmpdir = NULL;
	bool file_exists = false;
	m_create(char,symlinked_tmpdir,MEMORY_STRING);
	m_create(char,expected_directory_path,MEMORY_STRING);
	Return restore_status = SUCCESS;
	Return cleanup_status = SUCCESS;

	ASSERT(SUCCESS == save_tmpdir_value(&saved_tmpdir));
	ASSERT(SUCCESS == create_directory("0004_symlink_parent"));
	ASSERT(SUCCESS == create_directory("0004_symlink_parent/real_tmp_root"));
	ASSERT(SUCCESS == create_symlink("real_tmp_root","0004_symlink_parent/link_tmp_root"));
	ASSERT(SUCCESS == construct_path("0004_symlink_parent/link_tmp_root",symlinked_tmpdir));
	ASSERT(SUCCESS == set_environment_variable("TMPDIR",m_text(symlinked_tmpdir)));
	ASSERT(SUCCESS == create_directory("a/b"));

	restore_status = restore_tmpdir_value(saved_tmpdir);

	if(SUCCESS == restore_status)
	{
		if(SUCCESS == construct_path("0004_symlink_parent/real_tmp_root/a/b",expected_directory_path)
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

		cleanup_status = delete_path("0004_symlink_parent");
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
static Return test0004_2(void)
{
	INITTEST;

	char *saved_tmpdir = NULL;
	m_create(char,symlinked_tmpdir,MEMORY_STRING);
	Return restore_status = SUCCESS;
	Return cleanup_status = SUCCESS;

	ASSERT(SUCCESS == save_tmpdir_value(&saved_tmpdir));
	ASSERT(SUCCESS == create_directory("0004_symlink_bad_parent"));
	ASSERT(SUCCESS == truncate_file_to_zero_size("0004_symlink_bad_parent/file_target"));
	ASSERT(SUCCESS == create_symlink("file_target","0004_symlink_bad_parent/link_file_root"));
	ASSERT(SUCCESS == construct_path("0004_symlink_bad_parent/link_file_root",symlinked_tmpdir));
	ASSERT(SUCCESS == set_environment_variable("TMPDIR",m_text(symlinked_tmpdir)));
	ASSERT(FAILURE == create_directory("a/b"));

	restore_status = restore_tmpdir_value(saved_tmpdir);

	if(SUCCESS == restore_status)
	{
		cleanup_status = delete_path("0004_symlink_bad_parent");
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

/**
 * @brief Run create_directory() unit tests
 */
Return test0004(void)
{
	INITTEST;

	TEST(test0004_1,"create_directory() accepts a symlinked directory in TMPDIR");
	TEST(test0004_2,"create_directory() rejects a symlinked file in TMPDIR");

	RETURN_STATUS;
}
