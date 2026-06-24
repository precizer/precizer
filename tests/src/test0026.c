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
 * @brief Verify that an existing relative path is reported as accessible
 *
 * @details
 * Creates a readable temporary file, opens its parent directory, and passes
 * only the file name to file_check_access(). Because the file exists and can
 * be read, the function must return FILE_ACCESS_ALLOWED
 */
static Return test0026_1(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	m_create(char,relative_path,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));
	ASSERT(SUCCESS == m_copy_literal(relative_path,"test0026_accessible.txt"));

	char absolute_file_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,absolute_path,MEMORY_STRING);
	const int written = snprintf(absolute_file_path,
		sizeof(absolute_file_path),
		"%s/%s",
		tmpdir_path,
		m_text(relative_path));

	ASSERT(written > 0 && (size_t)written < sizeof(absolute_file_path));

	ASSERT(SUCCESS == m_copy_string(absolute_path,(size_t)written + 1U,absolute_file_path));
	ASSERT(SUCCESS == open_file_stream(absolute_path,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_ACCESS_ALLOWED);

	const int close_status = close(directory_fd);
	const int remove_file_status = remove(absolute_file_path);
	call(m_del(absolute_path));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(relative_path));
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
	m_create(char,relative_path,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));
	ASSERT(SUCCESS == m_copy_literal(relative_path,"test0026_missing.txt"));

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_NOT_FOUND);

	const int close_status = close(directory_fd);
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(relative_path));
	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify that an unreadable path is reported as FILE_ACCESS_DENIED
 *
 * @details
 * Creates a file and then removes every permission from its parent directory.
 * The file still exists, but the process cannot pass through the directory to
 * reach it. file_check_access() must therefore return FILE_ACCESS_DENIED
 * rather than FILE_NOT_FOUND
 */
static Return test0026_3(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	m_create(char,relative_path,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));
	ASSERT(SUCCESS == m_copy_literal(relative_path,"test0026_locked_dir/file.txt"));

	char locked_dir[PATH_MAX] = "";
	const int dir_written = snprintf(locked_dir,sizeof(locked_dir),"%s/%s",tmpdir_path,"test0026_locked_dir");

	ASSERT(dir_written > 0 && (size_t)dir_written < sizeof(locked_dir));

	ASSERT(mkdir(locked_dir,0700) == 0);

	char locked_file_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,locked_path,MEMORY_STRING);
	const int file_written = snprintf(locked_file_path,
		sizeof(locked_file_path),
		"%s/%s",
		tmpdir_path,
		m_text(relative_path));

	ASSERT(file_written > 0 && (size_t)file_written < sizeof(locked_file_path));

	ASSERT(SUCCESS == m_copy_string(locked_path,(size_t)file_written + 1U,locked_file_path));
	ASSERT(SUCCESS == open_file_stream(locked_path,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	ASSERT(chmod(locked_dir,0000) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_ACCESS_DENIED);

	const int close_status = close(directory_fd);
	const int restore_status = chmod(locked_dir,0700);
	const int remove_file_status = remove(locked_file_path);
	const int remove_dir_status = rmdir(locked_dir);

	call(m_del(locked_path));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(restore_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_dir_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(relative_path));
	call(m_del(tmpdir));

	RETURN_STATUS;
}

/**
 * @brief Verify that an unexpected access failure is reported as FILE_ACCESS_ERROR
 *
 * @details
 * Creates a real file and uses the test access-status hook to force
 * FILE_ACCESS_ERROR for that relative path. Because this error means neither
 * "missing" nor "permission denied", file_check_access() must return
 * FILE_ACCESS_ERROR
 */
static Return test0026_4(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	m_create(char,relative_path,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));
	ASSERT(SUCCESS == m_copy_literal(relative_path,"test0026_error.txt"));

	char error_path[PATH_MAX] = "";
	FILE *file = NULL;
	m_create(char,error_target,MEMORY_STRING);
	const int written = snprintf(error_path,
		sizeof(error_path),
		"%s/%s",
		tmpdir_path,
		m_text(relative_path));

	ASSERT(written > 0 && (size_t)written < sizeof(error_path));

	ASSERT(SUCCESS == m_copy_string(error_target,(size_t)written + 1U,error_path));
	ASSERT(SUCCESS == open_file_stream(error_target,"wb",&file));
	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_ACCESS_ALLOWED);

	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",m_text(relative_path)));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS","FILE_ACCESS_ERROR"));
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_ACCESS_ERROR);
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_SUFFIX",""));
	ASSERT(SUCCESS == set_environment_variable("TESTITALL_TEST_ENV_FILE_ACCESS_STATUS",""));

	const int close_status = close(directory_fd);
	const int remove_file_status = remove(error_path);
	call(m_del(error_target));
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(relative_path));
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
static Return test0026_5(void)
{
	INITTEST;

	m_create(char,tmpdir,MEMORY_STRING);
	m_create(char,relative_path,MEMORY_STRING);
	const char *tmpdir_path = "";

	ASSERT(SUCCESS == test0026_prepare_tmpdir(tmpdir,&tmpdir_path));
	ASSERT(SUCCESS == m_copy_literal(relative_path,"test0026_5_search_only.txt"));

	char absolute_file_path[PATH_MAX] = "";
	const int written = snprintf(absolute_file_path,
		sizeof(absolute_file_path),
		"%s/%s",
		tmpdir_path,
		m_text(relative_path));

	ASSERT(written > 0 && (size_t)written < sizeof(absolute_file_path));

	FILE *file = fopen(absolute_file_path,"wb");

	ASSERT(file != NULL);
	ASSERT(fclose(file) == 0);
	ASSERT(chmod(tmpdir_path,0111) == 0);

	int directory_fd = -1;

	ASSERT(directory_open(tmpdir,&directory_fd) == FILE_ACCESS_ALLOWED);
	ASSERT(file_check_access(directory_fd,relative_path,R_OK) == FILE_ACCESS_ALLOWED);

	const int close_status = close(directory_fd);
	const int restore_status = chmod(tmpdir_path,0700);
	const int remove_file_status = remove(absolute_file_path);
	const int remove_tmpdir_status = rmdir(tmpdir_path);

	ASSERT(close_status == 0);
	ASSERT(restore_status == 0);
	ASSERT(remove_file_status == 0);
	ASSERT(remove_tmpdir_status == 0);

	call(m_del(tmpdir));
	call(m_del(relative_path));

	RETURN_STATUS;
}

/**
 * @brief Test directory-relative file access checks
 *
 * @details
 * Covers readable, missing, permission-denied, and unexpected-error results for
 * paths checked relative to an opened directory. It also verifies that a
 * directory can be opened once and reused to check relative file names,
 * including a directory that can be searched but cannot be listed
 */
Return test0026(void)
{
	INITTEST;

	TEST(test0026_1,"file_check_access(): readable path relative to a directory descriptor");
	TEST(test0026_2,"file_check_access(): missing path relative to a directory descriptor");
	TEST(test0026_3,"file_check_access(): inaccessible path relative to a directory descriptor");
	TEST(test0026_4,"file_check_access(): forced access-check failure");
	TEST(test0026_5,"file_check_access(): readable path below a search-only directory");

	RETURN_STATUS;
}
