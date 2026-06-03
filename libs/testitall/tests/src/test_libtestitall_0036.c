#include "test_libtestitall_all.h"

static const char expected_construct_path_null_filename_stderr_pattern[] =
        "\\Aconstruct_path: filename must not be NULL\\Z";

static const char expected_construct_path_missing_tmpdir_stderr_pattern[] =
        "\\Aconstruct_path: TMPDIR is not set for \"tmpdir_missing\\.txt\"\\Z";

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
 * @brief Check construct_path() diagnostic output for invalid inputs
 *
 * @return Return status code
 */
Return test_libtestitall_0036(void)
{
	INITTEST;

	m_create(char,path,MEMORY_STRING);
	char *saved_tmpdir = NULL;
	Return saved_tmpdir_status = FAILURE;
	Return restore_tmpdir_status = SUCCESS;
	bool tmpdir_saved = false;

	call(m_del(STDERR));

	ASSERT(FAILURE == construct_path(NULL,path));
	ASSERT(SUCCESS == assert_stderr_matches_pattern(
		expected_construct_path_null_filename_stderr_pattern));

	saved_tmpdir_status = save_tmpdir_value(&saved_tmpdir);
	ASSERT(SUCCESS == saved_tmpdir_status);

	if(SUCCESS == saved_tmpdir_status)
	{
		tmpdir_saved = true;
	}

	if(SUCCESS == status)
	{
		ASSERT(unsetenv("TMPDIR") == 0);
	}

	call(m_del(STDERR));

	if(SUCCESS == status)
	{
		ASSERT(FAILURE == construct_path("tmpdir_missing.txt",path));
		ASSERT(SUCCESS == assert_stderr_matches_pattern(
			expected_construct_path_missing_tmpdir_stderr_pattern));
	}

	if(tmpdir_saved == true)
	{
		restore_tmpdir_status = restore_tmpdir_value(saved_tmpdir);
	}

	if(SUCCESS == status)
	{
		ASSERT(SUCCESS == restore_tmpdir_status);
	}

	free(saved_tmpdir);
	call(m_del(path));

	RETURN_STATUS;
}
