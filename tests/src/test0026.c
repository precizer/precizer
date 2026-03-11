#include "sute.h"
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/**
 * @brief Unit tests for file_check_access()
 *
 * @details
 * - Verifies readable absolute paths are detected immediately
 * - Verifies missing files return FILE_NOT_FOUND without errors
 * - Verifies unreadable paths return FILE_ACCESS_DENIED
 */
Return test0026(void)
{
	INITTEST;

	create(char,tmpdir);
	const char *tmpdir_path = getcstring(tmpdir);

	ASSERT(SUCCESS == create_tmpdir(tmpdir));
	tmpdir_path = getcstring(tmpdir);

	/* Absolute path should be reported readable */
	{
		char abs_path[PATH_MAX];
		FILE *file = NULL;
		create(char,absolute_path);
		const int written = snprintf(abs_path,sizeof(abs_path),"%s/%s",tmpdir_path,"test0026_abs.txt");
		ASSERT(written > 0 && (size_t)written < sizeof(abs_path));

		ASSERT(SUCCESS == copy_cstring(absolute_path,abs_path,strlen(abs_path) + 1U));
		ASSERT(SUCCESS == open_file_stream(absolute_path,"wb",&file));
		ASSERT(file != NULL);
		if(file != NULL)
		{
			ASSERT(fclose(file) == 0);
		}

		const size_t len = strlen(abs_path);
		FileAccessStatus rc = file_check_access(abs_path,len,R_OK);
		ASSERT(rc == FILE_ACCESS_ALLOWED);

		remove(abs_path);
		call(del(absolute_path));
	}

	/* Missing file should report not found without errors */
	{
		char missing_path[PATH_MAX];
		const int written = snprintf(missing_path,sizeof(missing_path),"%s/%s",tmpdir_path,"test0026_missing.txt");
		ASSERT(written > 0 && (size_t)written < sizeof(missing_path));
		remove(missing_path); // ensure absence

		const size_t len = strlen(missing_path);
		FileAccessStatus rc = file_check_access(missing_path,len,R_OK);

		ASSERT(rc == FILE_NOT_FOUND);
	}

	/* Unreadable path (directory without permissions) should report denied */
	{
		char locked_dir[PATH_MAX];
		const int dir_written = snprintf(locked_dir,sizeof(locked_dir),"%s/%s",tmpdir_path,"test0026_locked_dir");
		ASSERT(dir_written > 0 && (size_t)dir_written < sizeof(locked_dir));

		int mk_rc = mkdir(locked_dir,0700);
		ASSERT(mk_rc == 0);

		char locked_file_path[PATH_MAX];
		FILE *file = NULL;
		create(char,locked_path);
		const int file_written = snprintf(locked_file_path,sizeof(locked_file_path),"%s/%s",locked_dir,"file.txt");
		ASSERT(file_written > 0 && (size_t)file_written < sizeof(locked_file_path));

		ASSERT(SUCCESS == copy_cstring(locked_path,locked_file_path,strlen(locked_file_path) + 1U));
		ASSERT(SUCCESS == open_file_stream(locked_path,"wb",&file));
		ASSERT(file != NULL);
		if(file != NULL)
		{
			ASSERT(fclose(file) == 0);
		}

		/* lock directory */
		ASSERT(chmod(locked_dir,0000) == 0);

		const size_t len = strlen(locked_file_path);
		FileAccessStatus rc = file_check_access(locked_file_path,len,R_OK);

		/* restore permissions for cleanup */
		ASSERT(chmod(locked_dir,0700) == 0);
		ASSERT(remove(locked_file_path) == 0);
		ASSERT(rmdir(locked_dir) == 0);
		call(del(locked_path));

		ASSERT(rc == FILE_ACCESS_DENIED);
	}

	if(tmpdir_path != NULL && tmpdir_path[0] != '\0')
	{
		const int remove_tmpdir_status = rmdir(tmpdir_path);
		if(SUCCESS == status)
		{
			ASSERT(remove_tmpdir_status == 0);
		}
	}

	call(del(tmpdir));

	RETURN_STATUS;
}
