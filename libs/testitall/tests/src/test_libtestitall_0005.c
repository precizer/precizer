#include "test_libtestitall_all.h"
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief Check create_tmpdir() fallback and configured parent directory selection
 * @details Verifies fallback to P_tmpdir or /tmp when TMPDIR is unset. Verifies
 * TMPDIR is honored when it is set. Verifies each generated directory name
 * contains a non-empty prefix and an mkdtemp-compatible suffix
 *
 * @return Return status code
 */
Return test_libtestitall_0005(void)
{
	INITTEST;

	m_create(char,fallback_tmpdir,MEMORY_STRING);
	m_create(char,nested_tmpdir,MEMORY_STRING);
	char *saved_tmpdir = NULL;
	const char *original_tmpdir = getenv("TMPDIR");
	const char *expected_root = "/tmp";
	const char *fallback_path = NULL;
	const char *nested_path = NULL;
	int remove_nested_status = 0;
	int remove_fallback_status = 0;
	int restore_tmpdir_status = 0;

	#ifdef P_tmpdir
	IF(P_tmpdir[0] != '\0')
	{
		expected_root = P_tmpdir;
	}
	#endif

	IF(original_tmpdir != NULL)
	{
		saved_tmpdir = strdup(original_tmpdir);
		ASSERT(saved_tmpdir != NULL);
	}

	ASSERT(unsetenv("TMPDIR") == 0);

	ASSERT(SUCCESS == create_tmpdir(fallback_tmpdir));
	fallback_path = m_text(fallback_tmpdir);
	ASSERT(fallback_path != NULL);

	IF(SUCCESS == status)
	{
		const size_t expected_root_length = strlen(expected_root);
		const size_t fallback_path_length = strlen(fallback_path);
		const char *fallback_name_separator = strrchr(fallback_path,'/');
		const char *suffix_separator = strrchr(fallback_path,'.');

		ASSERT(expected_root_length > 0U);
		ASSERT(fallback_path_length > expected_root_length);
		ASSERT(0 == strncmp(fallback_path,expected_root,expected_root_length));
		ASSERT(fallback_name_separator != NULL);
		ASSERT(fallback_name_separator + ('/' == expected_root[expected_root_length - 1U])
			== fallback_path + expected_root_length);
		ASSERT(fallback_name_separator[1U] != '\0');
		ASSERT(suffix_separator != NULL);
		ASSERT(suffix_separator != fallback_name_separator + 1U);
		ASSERT(strlen(suffix_separator + 1U) == 6U);
		struct stat directory_stat;
		ASSERT(0 == stat(fallback_path,&directory_stat));
		ASSERT(S_ISDIR(directory_stat.st_mode));
		ASSERT(SUCCESS == set_environment_variable("TMPDIR",fallback_path));
	}

	ASSERT(SUCCESS == create_tmpdir(nested_tmpdir));
	nested_path = m_text(nested_tmpdir);
	ASSERT(nested_path != NULL);

	IF(SUCCESS == status)
	{
		const size_t prefix_length = strlen(fallback_path);
		const size_t nested_path_length = strlen(nested_path);
		const char *nested_name_separator = strrchr(nested_path,'/');
		const char *suffix_separator = strrchr(nested_path,'.');

		ASSERT(nested_path_length > prefix_length);
		ASSERT(0 == strncmp(nested_path,fallback_path,prefix_length));
		ASSERT(nested_name_separator != NULL);
		ASSERT(nested_name_separator == nested_path + prefix_length);
		ASSERT(nested_name_separator[1U] != '\0');
		ASSERT(suffix_separator != NULL);
		ASSERT(suffix_separator != nested_name_separator + 1U);
		ASSERT(strlen(suffix_separator + 1U) == 6U);
		struct stat directory_stat;
		ASSERT(0 == stat(nested_path,&directory_stat));
		ASSERT(S_ISDIR(directory_stat.st_mode));
	}

	IF(nested_path != NULL && nested_path[0] != '\0')
	{
		remove_nested_status = rmdir(nested_path);
	}

	IF(fallback_path != NULL && fallback_path[0] != '\0')
	{
		remove_fallback_status = rmdir(fallback_path);
	}

	IF(saved_tmpdir != NULL)
	{
		restore_tmpdir_status = setenv("TMPDIR",saved_tmpdir,1);
	} else {
		restore_tmpdir_status = unsetenv("TMPDIR");
	}

	ASSERT(remove_nested_status == 0);
	ASSERT(remove_fallback_status == 0);
	ASSERT(restore_tmpdir_status == 0);

	free(saved_tmpdir);

	call(m_del(nested_tmpdir));
	call(m_del(fallback_tmpdir));

	RETURN_STATUS;
}
