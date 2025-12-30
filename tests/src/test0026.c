#include "sute.h"
#include <limits.h>

/**
 * @brief Unit tests for file_check_access().
 *
 * @details
 * - Verifies readable absolute paths are detected immediately.
 * - Verifies relative paths are resolved via config->running_dir and marked readable.
 * - Verifies missing files return false without errors.
 */
Return test0026(void)
{
	INITTEST;

	const char *tmpdir = getenv("TMPDIR");
	ASSERT(tmpdir != NULL);

	/* Absolute path should be reported readable */
	{
		char abs_path[PATH_MAX];
		const int written = snprintf(abs_path,sizeof(abs_path),"%s/%s",tmpdir,"test0026_abs.txt");
		ASSERT(written > 0 && (size_t)written < sizeof(abs_path));

		FILE *f = fopen(abs_path,"w");
		ASSERT(f != NULL);
		fclose(f);

		bool readable = false;
		short unsigned int len = (short unsigned int)strlen(abs_path);

		Return rc = file_check_access(abs_path,len,&readable);
		ASSERT(rc == SUCCESS);
		ASSERT(readable == true);

		remove(abs_path);
	}

	/* Relative path should resolve using config->running_dir */
	{
		const char *relative_name = "test0026_rel.txt";
		char rel_full_path[PATH_MAX];
		const int written = snprintf(rel_full_path,sizeof(rel_full_path),"%s/%s",tmpdir,relative_name);
		ASSERT(written > 0 && (size_t)written < sizeof(rel_full_path));

		FILE *f = fopen(rel_full_path,"w");
		ASSERT(f != NULL);
		fclose(f);

		char *prev_dir = config->running_dir;
		long int prev_len = config->running_dir_size;

		config->running_dir = strdup(tmpdir);
		ASSERT(config->running_dir != NULL);
		config->running_dir_size = (long int)strlen(tmpdir) + 1; // includes terminating '\0' like determine_running_dir()

		bool readable = false;
		short unsigned int len = (short unsigned int)strlen(relative_name);

		Return rc = file_check_access(relative_name,len,&readable);

		free(config->running_dir);
		config->running_dir = prev_dir;
		config->running_dir_size = prev_len;
		remove(rel_full_path);

		ASSERT(rc == SUCCESS);
		ASSERT(readable == true);
	}

	/* Missing file should report not readable without errors */
	{
		char missing_path[PATH_MAX];
		const int written = snprintf(missing_path,sizeof(missing_path),"%s/%s",tmpdir,"test0026_missing.txt");
		ASSERT(written > 0 && (size_t)written < sizeof(missing_path));
		remove(missing_path); // ensure absence

		bool readable = true;
		short unsigned int len = (short unsigned int)strlen(missing_path);

		Return rc = file_check_access(missing_path,len,&readable);

		ASSERT(rc == SUCCESS);
		ASSERT(readable == false);
	}

	RETURN_STATUS;
}
