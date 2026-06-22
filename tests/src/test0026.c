#include "sute.h"
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/**
 * @brief Create the temporary directory used by one file access test
 *
 * The directory path is stored in @p tmpdir. A read-only view of the same path
 * is returned through @p tmpdir_path_out for use with standard filesystem calls
 *
 * @param[out] tmpdir Memory descriptor that receives the directory path
 * @param[out] tmpdir_path_out Receives a read-only view of the stored path
 * @return SUCCESS when the directory was created and both outputs are ready,
 *         otherwise FAILURE
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
 * Creates a readable temporary file and passes its complete path to
 * file_check_access_absolute(). Because the file exists and can be read, the
 * function must return FILE_ACCESS_ALLOWED
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

	ASSERT(file_check_access_absolute(abs_path,(size_t)written,R_OK) == FILE_ACCESS_ALLOWED);

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
 * Builds a path inside a temporary directory without creating a file at that
 * location. The access check must distinguish this normal missing-file case
 * from permission problems and unexpected filesystem errors
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

	remove(missing_path); // Make sure no leftover file can change the expected result

	ASSERT(file_check_access_absolute(missing_path,strlen(missing_path),R_OK) == FILE_NOT_FOUND);

	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify that an unreadable path is reported as FILE_ACCESS_DENIED
 *
 * @details
 * Creates a file and then removes every permission from its parent directory.
 * The file still exists, but the process cannot pass through the directory to
 * reach it. file_check_access_absolute() must therefore return
 * FILE_ACCESS_DENIED rather than FILE_NOT_FOUND
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

	ASSERT(file_check_access_absolute(locked_file_path,(size_t)file_written,R_OK) == FILE_ACCESS_DENIED);

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
 * Creates a real file and makes the test replacement for access() report EIO,
 * which represents a low-level input/output failure. Because this error means
 * neither "missing" nor "permission denied", file_check_access_absolute() must
 * return FILE_ACCESS_ERROR
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

	ASSERT(file_check_access_absolute(error_path,strlen(error_path),R_OK) == FILE_ACCESS_ERROR);

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
 * @brief Verify directory-relative access to an existing file
 *
 * Creates a file inside a temporary directory and opens that directory once.
 * The returned descriptor identifies the directory and is passed to
 * file_check_access() together with only the file name. A successful result
 * confirms that the relative name is resolved inside the opened directory
 * without constructing another absolute path
 */
static Return test0026_5(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";
	const char *relative_file_name = "test0026_5_relative.txt";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char absolute_file_path[PATH_MAX] = "";
	const int written = snprintf(absolute_file_path,
		sizeof(absolute_file_path),
		"%s/%s",
		tmpdir_path,
		relative_file_name);

	ASSERT(written > 0 && (size_t)written < sizeof(absolute_file_path));

	FILE *file = fopen(absolute_file_path,"wb");

	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir_path,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_file_name,R_OK) == FILE_ACCESS_ALLOWED);

	const int close_status = close(directory_fd);
	const int remove_file_status = remove(absolute_file_path);
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify directory-relative classification of a missing file
 *
 * Opens an empty temporary directory and checks a file name relative to its
 * descriptor. Because no file with that name exists inside the directory,
 * file_check_access() must return FILE_NOT_FOUND
 */
static Return test0026_6(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir_path,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,"test0026_6_missing_relative.txt",R_OK) == FILE_NOT_FOUND);

	const int close_status = close(directory_fd);
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify access through a directory that can be searched but not listed
 *
 * Creates a readable file and changes its parent directory mode to 0111. This
 * keeps permission to pass through the directory when a child name is already
 * known, but removes permission to list the directory contents. Opening the
 * directory and checking the known relative file name must still succeed,
 * proving that this workflow does not require directory read permission
 */
static Return test0026_7(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	const char *tmpdir_path = "";
	const char *relative_file_name = "test0026_7_search_only.txt";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));

	char absolute_file_path[PATH_MAX] = "";
	const int written = snprintf(absolute_file_path,
		sizeof(absolute_file_path),
		"%s/%s",
		tmpdir_path,
		relative_file_name);

	ASSERT(written > 0 && (size_t)written < sizeof(absolute_file_path));

	FILE *file = fopen(absolute_file_path,"wb");

	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);
	ASSERT(chmod(tmpdir_path,0111) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir_path,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_file_name,R_OK) == FILE_ACCESS_ALLOWED);

	const int close_status = close(directory_fd);
	const int restore_status = chmod(tmpdir_path,0700);
	const int remove_file_status = remove(absolute_file_path);
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(restore_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Test absolute and directory-relative file access checks
 *
 * @details
 * Covers readable, missing, permission-denied, and unexpected-error results for
 * absolute paths. It also verifies that a directory can be opened once and
 * reused to check relative file names, including a directory that can be
 * searched but cannot be listed
 */
Return test0026(void)
{
	INITTEST;

	TEST(test0026_1,"file_check_access_absolute(): readable absolute path");
	TEST(test0026_2,"file_check_access_absolute(): missing file");
	TEST(test0026_3,"file_check_access_absolute(): unreadable path");
#ifndef EVIL_EMPIRE_OS
	TEST(test0026_4,"file_check_access_absolute(): unexpected access failure");
#endif
	TEST(test0026_5,"file_check_access(): readable path relative to a directory descriptor");
	TEST(test0026_6,"file_check_access(): missing path relative to a directory descriptor");
	TEST(test0026_7,"file_check_access(): readable path below a search-only directory");

	RETURN_STATUS;
}
