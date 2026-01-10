#include "sute.h"
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/**
 * @brief Unit tests for file_check_access().
 *
 * @details
 * - Verifies readable absolute paths are detected immediately.
 * - Verifies relative paths are resolved via config->running_dir and marked readable.
 * - Verifies missing files return FILE_ACCESS_NOT_FOUND without errors.
 * - Verifies unreadable paths return FILE_ACCESS_DENIED.
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

		const size_t len = strlen(abs_path);
		FileAccessStatus rc = file_check_access(abs_path,len);
		ASSERT(rc == FILE_ACCESS_ALLOWED);

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

		const size_t len = strlen(relative_name);
		FileAccessStatus rc = file_check_access(relative_name,len);

		free(config->running_dir);
		config->running_dir = prev_dir;
		config->running_dir_size = prev_len;
		remove(rel_full_path);

		ASSERT(rc == FILE_ACCESS_ALLOWED);
	}

	/* Missing file should report not found without errors */
	{
		char missing_path[PATH_MAX];
		const int written = snprintf(missing_path,sizeof(missing_path),"%s/%s",tmpdir,"test0026_missing.txt");
		ASSERT(written > 0 && (size_t)written < sizeof(missing_path));
		remove(missing_path); // ensure absence

		const size_t len = strlen(missing_path);
		FileAccessStatus rc = file_check_access(missing_path,len);

		ASSERT(rc == FILE_ACCESS_NOT_FOUND);
	}

	/* Unreadable path (directory without permissions) should report denied */
	{
		char locked_dir[PATH_MAX];
		const int dir_written = snprintf(locked_dir,sizeof(locked_dir),"%s/%s",tmpdir,"test0026_locked_dir");
		ASSERT(dir_written > 0 && (size_t)dir_written < sizeof(locked_dir));

		int mk_rc = mkdir(locked_dir,0700);
		ASSERT(mk_rc == 0);

		char locked_file_path[PATH_MAX];
		const int file_written = snprintf(locked_file_path,sizeof(locked_file_path),"%s/%s",locked_dir,"file.txt");
		ASSERT(file_written > 0 && (size_t)file_written < sizeof(locked_file_path));

		FILE *f = fopen(locked_file_path,"w");
		ASSERT(f != NULL);
		fclose(f);

		/* lock directory */
		ASSERT(chmod(locked_dir,0000) == 0);

		const size_t len = strlen(locked_file_path);
		FileAccessStatus rc = file_check_access(locked_file_path,len);

		/* restore permissions for cleanup */
		ASSERT(chmod(locked_dir,0700) == 0);
		ASSERT(remove(locked_file_path) == 0);
		ASSERT(rmdir(locked_dir) == 0);

		ASSERT(rc == FILE_ACCESS_DENIED);
	}

	RETURN_STATUS;
}
