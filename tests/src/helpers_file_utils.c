#include "sute.h"
#include "monocypher-ed25519.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

/**
 * @brief Open a writable file stream with explicit create mode 0600
 *
 * Supports only `"ab"` and `"wb"` modes
 *
 * @param[in] file_path Managed path string passed directly to `open()`
 * @param[in] stream_open_mode Mode string for `fdopen()`
 * @param[out] opened_file_stream_out Output writable stream
 *
 * @return Return status code
 */
Return open_file_stream(
	const memory *file_path,
	const char   *stream_open_mode,
	FILE         **opened_file_stream_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int writable_file_descriptor = -1;
	int file_open_flags = 0;
	const char *file_path_string = NULL;

	if(file_path == NULL || stream_open_mode == NULL || opened_file_stream_out == NULL)
	{
		status = FAILURE;
	}

	IF(opened_file_stream_out != NULL)
	{
		*opened_file_stream_out = NULL;
	}

	if(SUCCESS == status)
	{
		file_path_string = m_text(file_path);
	}

	if(SUCCESS == status)
	{
		if(strcmp(stream_open_mode,"ab") == 0)
		{
			file_open_flags = O_WRONLY | O_CREAT | O_APPEND;
		} else if(strcmp(stream_open_mode,"wb") == 0){
			file_open_flags = O_WRONLY | O_CREAT | O_TRUNC;
		} else {
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		writable_file_descriptor = open(file_path_string,file_open_flags,0600);

		if(writable_file_descriptor < 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		*opened_file_stream_out = fdopen(writable_file_descriptor,stream_open_mode);

		if(*opened_file_stream_out == NULL)
		{
			(void)close(writable_file_descriptor);
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * @brief Verify access to a file path relative to a test root under TMPDIR
 *
 * Builds the absolute root path from TMPDIR, opens that root once, and checks
 * @p relative_path through file_check_access(). This mirrors the application's
 * current traversal access model while keeping tests compact
 *
 * @param[in] root_path_to_tmpdir Root directory path relative to TMPDIR
 * @param[in] relative_path Path to check relative to the opened root
 * @param[in] mode Access mode passed to file_check_access()
 * @param[in] expected_status Required access-check result
 *
 * @return SUCCESS when the access check matches @p expected_status, otherwise FAILURE
 */
Return expect_file_access_from_root(
	const char       *root_path_to_tmpdir,
	const char       *relative_path,
	const int        mode,
	FileAccessStatus expected_status)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int root_directory_fd = -1;

	m_create(char,root_path,MEMORY_STRING);
	m_create(char,relative_path_memory,MEMORY_STRING);

	if(root_path_to_tmpdir == NULL || relative_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(root_path_to_tmpdir,root_path);
	}

	if(SUCCESS == status)
	{
		status = m_copy_string(relative_path_memory,strlen(relative_path) + 1U,relative_path);
	}

	if(SUCCESS == status)
	{
		if(directory_open(root_path,&root_directory_fd) != FILE_ACCESS_ALLOWED)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const FileAccessStatus access_status = file_check_access(
			root_directory_fd,
			relative_path_memory,
			mode);

		if(access_status != expected_status)
		{
			status = FAILURE;
		}
	}

	if(root_directory_fd >= 0)
	{
		if(close(root_directory_fd) != 0)
		{
			status = FAILURE;
		}
	}

	m_del(relative_path_memory);
	m_del(root_path);

	return(status);
}

/**
 * @brief Truncate an existing file to zero bytes by reopening it in binary write mode
 *
 * @param[in] relative_path_to_tmpdir Relative path from TMPDIR to the target file
 *
 * @return Return status code:
 *         - SUCCESS: File was truncated successfully
 *         - FAILURE: Path construction or file operation failed
 */
Return truncate_file_to_zero_size(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;

	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		status = open_file_stream(
			absolute_path,
			"wb",
			&file);
	}

	IF(file != NULL && fclose(file) != 0)
	{
		status = FAILURE;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Create symbolic link by path relative to TMPDIR
 *
 * The target string is stored in the link exactly as provided
 *
 * @param[in] link_target Literal target string for the symbolic link
 * @param[in] relative_link_path_to_tmpdir Symbolic link path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Symbolic link was created successfully
 *         - FAILURE: Validation, path construction, or symlink creation failed
 */
Return create_symlink(
	const char *link_target,
	const char *relative_link_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	m_create(char,absolute_path,MEMORY_STRING);

	if(link_target == NULL || relative_link_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_link_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status && symlink(link_target,m_text(absolute_path)) != 0)
	{
		status = FAILURE;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Change file mode by path relative to TMPDIR
 *
 * The provided mode is applied exactly with native `chmod()`
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 * @param[in] new_mode Exact mode value to apply
 *
 * @return Return status code:
 *         - SUCCESS: File mode was changed successfully
 *         - FAILURE: Validation, path construction, or chmod failed
 */
Return change_mode(
	const char   *relative_path_to_tmpdir,
	unsigned int new_mode)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status && chmod(m_text(absolute_path),(mode_t)new_mode) != 0)
	{
		status = FAILURE;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Remove a file or directory tree when it exists
 *
 * This helper is intended for cleanup paths that may or may not have been
 * created before a test scenario stopped. Missing paths are accepted. Any
 * other filesystem error is reported as a failure so stale test state is not
 * silently ignored
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Path was absent or removed successfully
 *         - FAILURE: Validation, path construction, stat, or removal failed
 */
Return delete_path_if_present(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat path_stat = {0};
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		errno = 0;

		if(lstat(m_text(absolute_path),&path_stat) == 0)
		{
			status = delete_path(relative_path_to_tmpdir);

		} else if(errno != ENOENT){
			status = FAILURE;
		}
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Prepare an isolated working copy of the huge-file fixture
 *
 * The fixture is copied from ORIGIN_DIR into TMPDIR and the output path points
 * to the copied `hugetestfile`. The optional stat output lets tests assert
 * interruption offsets against the real file size
 *
 * @param[out] huge_file_path Absolute path to the copied fixture file
 * @param[out] huge_file_stat_out Optional file metadata read from the copied fixture
 *
 * @return Return status code:
 *         - SUCCESS: Fixture copy is ready and output path was filled
 *         - FAILURE: Environment setup, copy, path construction, or stat failed
 */
Return prepare_huge_fixture(
	memory      *huge_file_path,
	struct stat *huge_file_stat_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat huge_file_stat = {0};
	static const char huge_fixture_root[] = "tests/fixtures/huge";
	static const char huge_fixture_file[] = "tests/fixtures/huge/hugetestfile";

	if(huge_file_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = set_environment_variable("TESTING","true");
	}

	if(SUCCESS == status)
	{
		status = create_directory("tests/fixtures");
	}

	if(SUCCESS == status)
	{
		status = copy_from_origin(huge_fixture_root,huge_fixture_root,REQUIRE_SOURCE_EXISTS);
	}

	if(SUCCESS == status)
	{
		status = construct_path(huge_fixture_file,huge_file_path);
	}

	if(SUCCESS == status && stat(m_text(huge_file_path),&huge_file_stat) != 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && huge_file_stat.st_size <= 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && huge_file_stat_out != NULL)
	{
		*huge_file_stat_out = huge_file_stat;
	}

	return(status);
}

/**
 * @brief Move file or directory by path relative to TMPDIR using native rename
 *
 * Empty source or destination path resolves to TMPDIR root
 *
 * @param[in] relative_source_path Source file or directory path relative to TMPDIR
 * @param[in] relative_destination_path Destination file or directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Source path was moved to destination path
 *         - FAILURE: Validation, path resolution, or native rename failed
 */
Return move_path(
	const char *relative_source_path,
	const char *relative_destination_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,source_absolute_path,MEMORY_STRING);
	m_create(char,destination_absolute_path,MEMORY_STRING);

	if(relative_source_path == NULL || relative_destination_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_source_path,source_absolute_path);
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_destination_path,destination_absolute_path);
	}

	if(SUCCESS == status && rename(m_text(source_absolute_path),m_text(destination_absolute_path)) != 0)
	{
		status = FAILURE;
	}

	m_del(destination_absolute_path);
	m_del(source_absolute_path);

	return(status);
}

/**
 * @brief Build the hidden backup path for a mutable fixture
 *
 * The returned path mirrors the fixture path under `.fixture_backups`
 *
 * @param[in] fixture_path Fixture path relative to TMPDIR
 * @param[out] backup_path_out Output hidden backup path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Hidden backup path was constructed
 *         - FAILURE: Validation or string construction failed
 */
static Return construct_mutable_fixture_backup_path(
	const char *fixture_path,
	memory     *backup_path_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	static const char mutable_fixture_backup_root[] = ".fixture_backups";

	if(fixture_path == NULL || backup_path_out == NULL || fixture_path[0] == '\0')
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = m_copy_string(backup_path_out,
			sizeof(mutable_fixture_backup_root),
			mutable_fixture_backup_root);
	}

	if(SUCCESS == status)
	{
		status = m_to_string(backup_path_out);
	}

	if(SUCCESS == status)
	{
		status = m_concat_literal(backup_path_out,"/");
	}

	if(SUCCESS == status)
	{
		status = m_concat_string(backup_path_out,strlen(fixture_path) + 1U,fixture_path);
	}

	return(status);
}

/**
 * @brief Derive the parent directory of a mutable-fixture backup path
 *
 * @param[in] backup_path Hidden backup path relative to TMPDIR
 * @param[out] backup_parent_path_out Output parent directory relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Parent directory path was derived
 *         - FAILURE: Validation, copy, or parent extraction failed
 */
static Return construct_mutable_fixture_backup_parent_path(
	const memory *backup_path,
	memory       *backup_parent_path_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *backup_parent_path_string = NULL;

	if(backup_path == NULL || backup_parent_path_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = m_copy(backup_parent_path_out,backup_path);
	}

	if(SUCCESS == status)
	{
		backup_parent_path_string = m_data(char,backup_parent_path_out);

		if(backup_parent_path_string == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		char *last_separator = strrchr(backup_parent_path_string,'/');

		if(last_separator == NULL)
		{
			status = FAILURE;
		} else {
			*last_separator = '\0';
		}
	}

	return(status);
}

/**
 * @brief Prepare a mutable working copy for a fixture path relative to TMPDIR
 *
 * The pristine backup is stored under the hidden `.fixture_backups` root
 *
 * @param[in] fixture_path Fixture path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Pristine backup was created and working copy was restored
 *         - FAILURE: Validation, backup path construction, or fixture copy failed
 */
Return prepare_mutable_fixture(const char *fixture_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,backup_path,MEMORY_STRING);
	m_create(char,backup_parent_path,MEMORY_STRING);

	if(fixture_path == NULL || fixture_path[0] == '\0')
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_mutable_fixture_backup_path(fixture_path,backup_path);
	}

	if(SUCCESS == status)
	{
		status = construct_mutable_fixture_backup_parent_path(backup_path,backup_parent_path);
	}

	if(SUCCESS == status)
	{
		status = create_directory(m_text(backup_parent_path));
	}

	if(SUCCESS == status)
	{
		status = move_path(fixture_path,m_text(backup_path));
	}

	if(SUCCESS == status)
	{
		status = copy_path(m_text(backup_path),fixture_path);
	}

	m_del(backup_parent_path);
	m_del(backup_path);

	return(status);
}

/**
 * @brief Restore a fixture path from the hidden mutable-fixture backup
 *
 * The working copy is deleted before the pristine backup is moved back
 *
 * @param[in] fixture_path Fixture path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Working copy was removed and pristine backup was restored
 *         - FAILURE: Validation, backup path construction, or restore failed
 */
Return restore_mutable_fixture(const char *fixture_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,backup_path,MEMORY_STRING);

	if(fixture_path == NULL || fixture_path[0] == '\0')
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_mutable_fixture_backup_path(fixture_path,backup_path);
	}

	if(SUCCESS == status)
	{
		status = delete_path(fixture_path);
	}

	if(SUCCESS == status)
	{
		status = move_path(m_text(backup_path),fixture_path);
	}

	m_del(backup_path);

	return(status);
}

/**
 * @brief Make a file sparse by extending logical size while keeping allocated blocks unchanged
 *
 * @param[in] relative_path_to_tmpdir Relative path from TMPDIR to the target file
 * @param[out] new_size_out Output for the new logical size after sparse growth
 * @param[out] blocks_after_change_out Output for allocated blocks after sparse growth
 *
 * @return Return status code:
 *         - SUCCESS: Sparse size change completed and outputs were filled
 *         - FAILURE: Validation or filesystem operation failed
 */
Return make_sparse_size_change_without_allocated_block_growth(
	const char *relative_path_to_tmpdir,
	off_t      *new_size_out,
	blkcnt_t   *blocks_after_change_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat before_stat = {0};
	struct stat after_stat = {0};
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL || new_size_out == NULL || blocks_after_change_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(m_text(absolute_path),&before_stat);
	}

	if(SUCCESS == status)
	{
		const off_t grown_size = before_stat.st_size + (off_t)131072;

		// Grow logical size via truncate to create a sparse tail without writing payload bytes
		if(grown_size <= before_stat.st_size)
		{
			status = FAILURE;
		} else if(truncate(m_text(absolute_path),grown_size) != 0){
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(m_text(absolute_path),&after_stat);
	}

	if(SUCCESS == status && after_stat.st_size <= before_stat.st_size)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && after_stat.st_blocks != before_stat.st_blocks)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		*new_size_out = after_stat.st_size;
		*blocks_after_change_out = after_stat.st_blocks;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Rewrite file content with dense bytes while preserving logical size
 *
 * @param[in] relative_path_to_tmpdir Relative path from TMPDIR to the target file
 * @param[in] target_size Logical size to keep after rewrite
 * @param[in] blocks_before_rewrite Allocated blocks before rewrite
 *
 * @return Return status code:
 *         - SUCCESS: Dense rewrite completed with unchanged logical size and a different allocated block count
 *         - FAILURE: Validation or filesystem operation failed
 */
Return rewrite_file_dense_with_same_size(
	const char     *relative_path_to_tmpdir,
	const off_t    target_size,
	const blkcnt_t blocks_before_rewrite)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	struct stat after_stat = {0};
	unsigned char buffer[4096];
	m_create(char,absolute_path,MEMORY_STRING);

	memset(buffer,'X',sizeof(buffer));

	if(relative_path_to_tmpdir == NULL || target_size <= 0)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);
	}

	if(SUCCESS == status)
	{
		// Rewrite the whole file with real bytes while keeping the same logical size
		status = open_file_stream(
			absolute_path,
			"wb",
			&file);
	}

	off_t written = 0;

	while(SUCCESS == status && written < target_size)
	{
		const off_t remaining = target_size - written;
		size_t chunk = sizeof(buffer);

		if(remaining < (off_t)chunk)
		{
			chunk = (size_t)remaining;
		}

		if(fwrite(buffer,sizeof(unsigned char),chunk,file) != chunk)
		{
			status = FAILURE;
		} else {
			written += (off_t)chunk;
		}
	}

	IF(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(m_text(absolute_path),&after_stat);
	}

	if(SUCCESS == status && after_stat.st_size != target_size)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && after_stat.st_blocks == blocks_before_rewrite)
	{
		status = FAILURE;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Compute SHA512 for a file using Monocypher
 *
 * @param[in] file_path File path passed directly to `fopen()`
 * @param[out] sha512_out Output SHA512 digest buffer
 *
 * @return Return status code:
 *         - SUCCESS: SHA512 digest computed successfully
 *         - FAILURE: Validation, I/O, or digest-size check failed
 */
Return compute_file_sha512_monocypher(
	const char    *file_path,
	unsigned char *sha512_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	crypto_sha512_ctx context = {0};
	unsigned char buffer[65536];

	if(file_path == NULL || sha512_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && SHA512_DIGEST_LENGTH != 64)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		file = fopen(file_path,"rb");

		if(file == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		crypto_sha512_init(&context);
	}

	while(SUCCESS == status)
	{
		const size_t bytes_read = fread(buffer,sizeof(unsigned char),sizeof(buffer),file);

		if(bytes_read == 0U)
		{
			if(ferror(file) != 0)
			{
				status = FAILURE;
			}
			break;
		}

		crypto_sha512_update(&context,buffer,bytes_read);
	}

	IF(SUCCESS == status)
	{
		crypto_sha512_final(&context,sha512_out);
	}

	IF(file != NULL)
	{
		(void)fclose(file);
	}

	return(status);
}

/**
 * @brief Append one byte to a file using native C file I/O
 *
 * @param[in] file_path_buffer Managed path string passed directly to `open_file_stream()`
 * @param[in] byte Byte value to append
 *
 * @return Return status code:
 *         - SUCCESS: Byte appended successfully
 *         - FAILURE: Validation or I/O failed
 */
Return append_byte_to_file(
	const memory  *file_path_buffer,
	unsigned char byte)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;

	if(file_path_buffer == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = open_file_stream(
			file_path_buffer,
			"ab",
			&file);
	}

	if(SUCCESS == status)
	{
		if(fwrite(&byte,sizeof(unsigned char),1U,file) != 1U)
		{
			status = FAILURE;
		}
	}

	IF(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	return(status);
}

/**
 * @brief Write string to file with append or replace mode
 *
 * The function writes bytes exactly as provided and never appends a newline
 *
 * @param[in] file_content String payload to write
 * @param[in] file_path File path relative to TMPDIR
 * @param[in] write_flags One of FILE_WRITE_APPEND or FILE_WRITE_REPLACE
 *
 * @return Return status code:
 *         - SUCCESS: The string bytes were written successfully
 *         - FAILURE: Validation, path resolution, or file I/O failed
 */
Return write_string_to_file(
	const char         *file_content,
	const char         *file_path,
	const unsigned int write_flags)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	const char *open_mode = NULL;
	m_create(char,absolute_path,MEMORY_STRING);

	if(file_content == NULL || file_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		if(write_flags == FILE_WRITE_APPEND)
		{
			open_mode = "ab";
		} else if(write_flags == FILE_WRITE_REPLACE){
			open_mode = "wb";
		} else {
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = construct_path(file_path,absolute_path);
	}

	if(SUCCESS == status)
	{
		status = open_file_stream(
			absolute_path,
			open_mode,
			&file);
	}

	if(SUCCESS == status)
	{
		const size_t bytes_to_write = strlen(file_content);

		if(bytes_to_write > 0U
		        && fwrite(file_content,sizeof(char),bytes_to_write,file) != bytes_to_write)
		{
			status = FAILURE;
		}
	}

	IF(file != NULL && fclose(file) != 0)
	{
		status = FAILURE;
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Append string bytes to file without newline
 *
 * @param[in] file_content String payload to append
 * @param[in] file_path File path relative to TMPDIR
 *
 * @return Return status code
 */
Return add_string_to(
	const char *file_content,
	const char *file_path)
{
	return(write_string_to_file(file_content,file_path,FILE_WRITE_APPEND));
}

/**
 * @brief Replace file content with string bytes without newline
 *
 * @param[in] file_content String payload to write
 * @param[in] file_path File path relative to TMPDIR
 *
 * @return Return status code
 */
Return replase_to_string(
	const char *file_content,
	const char *file_path)
{
	return(write_string_to_file(file_content,file_path,FILE_WRITE_REPLACE));
}

/**
 * @brief Set target file mtime to source mtime plus a nanosecond delta using native POSIX calls
 *
 * Relative paths are resolved from TMPDIR with construct_path
 * Source and target can be the same file
 * If relative_source_path is NULL, relative_target_path is used as source
 * If relative_target_path is NULL, the function returns FAILURE
 * The delta is applied in nanoseconds and can be any signed integer value
 * If mtime_delta_nanoseconds is 0, target mtime is set to source mtime
 * atime is preserved with UTIME_OMIT
 * Even when resulting mtime equals current mtime, successful metadata update may still change ctime
 * ctime cannot be set directly from userspace and will change automatically after metadata update
 *
 * @param[in] relative_source_path Relative path from TMPDIR to source file or NULL
 * @param[in] relative_target_path Relative path from TMPDIR to target file
 * @param[in] mtime_delta_nanoseconds Signed nanosecond delta applied to source mtime
 *
 * @return Return status code:
 *         - SUCCESS: Target mtime was updated
 *         - FAILURE: Validation, path resolution, stat, normalization, or timestamp update failed
 */
Return touch_file_mtime_with_reference_delta_ns(
	const char *relative_source_path,
	const char *relative_target_path,
	int64_t    mtime_delta_nanoseconds)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat source_file_stat = {0};
	struct timespec target_times[2] = {{0}};
	const char *source_relative_path = relative_source_path;
	m_create(char,source_absolute_path,MEMORY_STRING);
	m_create(char,target_absolute_path,MEMORY_STRING);

	// Require a target path because the mtime update is applied to this file
	if(relative_target_path == NULL)
	{
		status = FAILURE;
	}

	// Reuse target as source when source path is not provided
	if(SUCCESS == status && source_relative_path == NULL)
	{
		source_relative_path = relative_target_path;
	}

	// Resolve source path relative to TMPDIR
	if(SUCCESS == status)
	{
		status = construct_path(source_relative_path,source_absolute_path);
	}

	// Resolve target path relative to TMPDIR
	if(SUCCESS == status)
	{
		status = construct_path(relative_target_path,target_absolute_path);
	}

	// Read source stat to use its mtime as the reference point
	if(SUCCESS == status)
	{
		status = get_file_stat(m_text(source_absolute_path),&source_file_stat);
	}

	// Build target mtime by applying and normalizing nanosecond delta
	if(SUCCESS == status)
	{
		const intmax_t nanoseconds_per_second = 1000000000;
		intmax_t target_seconds = (intmax_t)source_file_stat.st_mtim.tv_sec
		        + (intmax_t)(mtime_delta_nanoseconds / nanoseconds_per_second);
		long target_nanoseconds = source_file_stat.st_mtim.tv_nsec
		        + (long)(mtime_delta_nanoseconds % nanoseconds_per_second);

		// Normalize nanosecond overflow into next second
		if(target_nanoseconds >= (long)nanoseconds_per_second)
		{
			target_nanoseconds -= (long)nanoseconds_per_second;
			target_seconds++;
			// Normalize negative nanoseconds by borrowing one second
		} else if(target_nanoseconds < 0){
			target_nanoseconds += (long)nanoseconds_per_second;
			target_seconds--;
		}

		time_t normalized_target_seconds = (time_t)target_seconds;

		// Ensure computed seconds value is representable as time_t
		if((intmax_t)normalized_target_seconds != target_seconds)
		{
			status = FAILURE;
		} else {
			// Keep atime unchanged and prepare mtime for utimensat
			target_times[0].tv_nsec = UTIME_OMIT;
			target_times[1].tv_sec = normalized_target_seconds;
			target_times[1].tv_nsec = target_nanoseconds;
		}
	}

	// Apply the prepared timestamp values to the target file
	if(SUCCESS == status)
	{
		if(utimensat(0,m_text(target_absolute_path),target_times,0) != 0)
		{
			status = FAILURE;
		}
	}

	m_del(source_absolute_path);
	m_del(target_absolute_path);

	return(status);
}

/**
 * @brief Modify first two bytes of a file and restore atime/mtime best effort
 *
 * @param[in] relative_path Relative path from TMPDIR to the target file
 *
 * @return Return status code:
 *         - SUCCESS: File bytes were modified
 *         - FAILURE: Validation or filesystem operation failed
 */
Return tamper_locked_file_bytes(const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int fd = -1;
	struct stat before = {0};
	unsigned char buffer[2] = {0};
	m_create(char,file_path,MEMORY_STRING);

	// Validate input path before any filesystem operations
	if(SUCCESS == status && relative_path == NULL)
	{
		status = FAILURE;
	}

	// Resolve path relative to TMPDIR
	if(SUCCESS == status)
	{
		status = construct_path(relative_path,file_path);
	}

	// Open file for in-place read and write operations
	if(SUCCESS == status && (fd = open(m_text(file_path),O_RDWR)) < 0)
	{
		status = FAILURE;
	}

	// Read file metadata to preserve timestamps later
	if(SUCCESS == status && fstat(fd,&before) != 0)
	{
		status = FAILURE;
	}

	// Require at least two bytes because exactly two bytes are modified
	if(SUCCESS == status && before.st_size < (off_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	// Read first two bytes that will be modified
	if(SUCCESS == status && pread(fd,buffer,sizeof(buffer),0) != (ssize_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	// Flip both bytes to guarantee content and checksum change
	if(SUCCESS == status)
	{
		buffer[0] = (unsigned char)~buffer[0];
		buffer[1] = (unsigned char)~buffer[1];
	}

	// Write modified bytes back to file start
	if(SUCCESS == status && pwrite(fd,buffer,sizeof(buffer),0) != (ssize_t)sizeof(buffer))
	{
		status = FAILURE;
	}

	// Restore atime and mtime best effort after content tampering
	if(SUCCESS == status)
	{
		struct timespec times[2] = {{0}};

		// Best effort restore for atime and mtime while ctime still changes on POSIX
		times[0] = before.st_atim;
		times[1] = before.st_mtim;

		if(futimens(fd,times) != 0)
		{
			status = FAILURE;
		}
	}

	// Close descriptor on all paths where open succeeded
	if(fd >= 0)
	{
		(void)close(fd);
	}

	m_del(file_path);

	return(status);
}
