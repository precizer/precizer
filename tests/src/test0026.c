#include "sute.h"
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/**
 * @brief Create a temporary directory for an individual file_check_access() subtest
 *
 * @param tmpdir Memory descriptor that receives the temporary directory path
 * @param tmpdir_path_out Receives a read-only pointer to the stored path
 * @return SUCCESS when the directory was created and exposed through tmpdir_path_out, otherwise FAILURE
 */
static Return test0026_prepare_tmpdir(
	memory     *tmpdir,
	const char **tmpdir_path_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	status = create_tmpdir(tmpdir);
	*tmpdir_path_out = m_text(tmpdir);

	provide(status);
}

/**
 * @brief Verify that an existing absolute path is reported as accessible
 *
 * @details
 * Creates a real temporary file and checks the simplest happy path. When the
 * file exists and the process can read it, file_check_access() should return
 * FILE_ACCESS_ALLOWED without needing any fallback or error classification
 */
static Return test0026_1(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char abs_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,absolute_path,MEMORY_STRING);
	const int written = snprintf(abs_path,sizeof(abs_path),"%s/%s",tmpdir_path,"test0026_abs.txt");

	ASSERT(written > 0 && (size_t)written < sizeof(abs_path));

	ASSERT(SUCCESS == m_copy_string(absolute_path,(size_t)written + 1U,abs_path));
	ASSERT(SUCCESS == open_file_stream(absolute_path,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	ASSERT(file_check_access(abs_path,(size_t)written,R_OK) == FILE_ACCESS_ALLOWED);

	const int remove_file_status = remove(abs_path);
	call(m_del(absolute_path));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify that a missing file is reported as FILE_NOT_FOUND
 *
 * @details
 * Builds a path inside a real temporary directory and makes sure that no file
 * exists there. This checks that a normal missing file is reported as
 * FILE_NOT_FOUND, not as a generic access error
 */
static Return test0026_2(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char missing_path[PATH_MAX] = "";
	const int written = snprintf(missing_path,sizeof(missing_path),"%s/%s",tmpdir_path,"test0026_missing.txt");

	ASSERT(written > 0 && (size_t)written < sizeof(missing_path));

	remove(missing_path); // ensure absence

	ASSERT(file_check_access(missing_path,strlen(missing_path),R_OK) == FILE_NOT_FOUND);

	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify that an unreadable path is reported as FILE_ACCESS_DENIED
 *
 * @details
 * Creates a file inside a directory and then removes permissions from that
 * directory. This simulates a path that exists but cannot be traversed, so
 * file_check_access() should report FILE_ACCESS_DENIED
 */
static Return test0026_3(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char locked_dir[PATH_MAX] = "";
	const int dir_written = snprintf(locked_dir,sizeof(locked_dir),"%s/%s",tmpdir_path,"test0026_locked_dir");

	ASSERT(dir_written > 0 && (size_t)dir_written < sizeof(locked_dir));

	ASSERT(mkdir(locked_dir,0700) == 0);

	char locked_file_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,locked_path,MEMORY_STRING);
	const int file_written = snprintf(locked_file_path,sizeof(locked_file_path),"%s/%s",locked_dir,"file.txt");

	ASSERT(file_written > 0 && (size_t)file_written < sizeof(locked_file_path));

	ASSERT(SUCCESS == m_copy_string(locked_path,(size_t)file_written + 1U,locked_file_path));
	ASSERT(SUCCESS == open_file_stream(locked_path,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	ASSERT(chmod(locked_dir,0000) == 0);

	ASSERT(file_check_access(locked_file_path,(size_t)file_written,R_OK) == FILE_ACCESS_DENIED);

	const int restore_status = chmod(locked_dir,0700);
	const int remove_file_status = remove(locked_file_path);
	const int remove_dir_status = rmdir(locked_dir);

	call(m_del(locked_path));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(restore_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_dir_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Verify that an unexpected access failure is reported as FILE_ACCESS_ERROR
 *
 * @details
 * Creates a real file and then uses the access() mock to force an unexpected
 * EIO failure for that path. This checks the defensive branch where the file
 * is not simply missing or denied, and file_check_access() must report
 * FILE_ACCESS_ERROR
 */
static Return test0026_4(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char error_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,error_target,MEMORY_STRING);
	size_t access_call_count = 0;
	const int written = snprintf(error_path,sizeof(error_path),"%s/%s",tmpdir_path,"test0026_error.txt");

	ASSERT(written > 0 && (size_t)written < sizeof(error_path));

	ASSERT(SUCCESS == m_copy_string(error_target,(size_t)written + 1U,error_path));
	ASSERT(SUCCESS == open_file_stream(error_target,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	mocks_access_reset();
	mocks_access_set_target_suffix(error_path);
	mocks_access_set_errno(EIO);
	mocks_access_enable(true);

	ASSERT(file_check_access(error_path,strlen(error_path),R_OK) == FILE_ACCESS_ERROR);

	mocks_access_enable(false);
	access_call_count = mocks_access_call_count();
	mocks_access_reset();

	const int remove_file_status = remove(error_path);
	call(m_del(error_target));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(access_call_count > 0U);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}
#endif

/**
 * @brief Unit tests for file_check_access()
 *
 * @details
 * Runs numbered subtests for the main outcomes that callers rely on: an
 * accessible file, a missing file, a permission-denied path, and an unexpected
 * low-level access failure
 */
Return test0026(void)
{
	INITTEST;

	TEST(test0026_1,"file_check_access(): readable absolute path");
	TEST(test0026_2,"file_check_access(): missing file");
	TEST(test0026_3,"file_check_access(): unreadable path");
#ifndef EVIL_EMPIRE_OS
	TEST(test0026_4,"file_check_access(): unexpected access failure");
#endif

	RETURN_STATUS;
}
