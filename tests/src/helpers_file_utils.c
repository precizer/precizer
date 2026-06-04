#include "sute.h"
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdlib.h>

static const char *copy_source_root_for_nftw = NULL;
static const char *copy_destination_root_for_nftw = NULL;
static size_t copy_source_root_length_for_nftw = 0U;
static size_t copy_destination_root_length_for_nftw = 0U;
static const char mutable_fixture_backup_root[] = ".fixture_backups";

/**
 * @brief Reset nftw copy context to an empty state
 */
static void reset_nftw_copy_context(void)
{
	copy_source_root_for_nftw = NULL;
	copy_destination_root_for_nftw = NULL;
	copy_source_root_length_for_nftw = 0U;
	copy_destination_root_length_for_nftw = 0U;
}

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
 * @brief Apply source mtime to destination path using utimensat
 *
 * @param[in] destination_path Destination filesystem path
 * @param[in] source_stat Source stat metadata
 * @param[in] utimensat_flags Flags forwarded to utimensat
 *
 * @return 0 on success, non-zero on failure
 */
static int apply_mtime_from_source_stat(
	const char        *destination_path,
	const struct stat *source_stat,
	const int         utimensat_flags)
{
	int status = 0;

	if(destination_path == NULL || source_stat == NULL)
	{
		status = -1;
	}

	if(status == 0)
	{
		struct timespec times[2] = {0};

		times[0].tv_nsec = UTIME_OMIT;
		times[1] = source_stat->st_mtim;

		if(utimensat(0,destination_path,times,utimensat_flags) != 0)
		{
			status = -1;
		}
	}

	return(status);
}

/**
 * @brief Remove callback used by nftw
 *
 * @param[in] path Current path from nftw walk
 * @param[in] stat_buffer Unused file stat pointer from nftw
 * @param[in] type_flag Unused type flag from nftw
 * @param[in] ftw_buffer Unused nftw frame data
 *
 * @return 0 on success, non-zero on failure
 */
static int nftw_remove_callback(
	const char        *path,
	const struct stat *stat_buffer,
	const int         type_flag,
	struct FTW        *ftw_buffer)
{
	(void)stat_buffer;
	(void)type_flag;
	(void)ftw_buffer;

	if(remove(path) != 0)
	{
		echo(STDERR,"delete_path: remove failed for %s: %s\n",path,strerror(errno));
		return(-1);
	}

	return(0);
}

/**
 * @brief Build destination path for nftw copy callback
 *
 * @param[in] source_path Current source path from nftw
 * @param[out] destination_path_out Destination path descriptor
 *
 * @return 0 on success, non-zero on failure
 */
static int build_copy_destination_path_for_nftw(
	const char *source_path,
	memory     *destination_path_out)
{
	int status = 0;

	if(source_path == NULL
	        || destination_path_out == NULL
	        || copy_source_root_for_nftw == NULL
	        || copy_destination_root_for_nftw == NULL)
	{
		status = -1;
	}

	if(status == 0 && strncmp(source_path,
		copy_source_root_for_nftw,
		copy_source_root_length_for_nftw) != 0)
	{
		status = -1;
	}

	if(status == 0)
	{
		const char *relative_path = source_path + copy_source_root_length_for_nftw;

		if(relative_path[0] == '/')
		{
			relative_path++;
		}

		if(m_copy_string(destination_path_out,
			copy_destination_root_length_for_nftw + 1U,
			copy_destination_root_for_nftw) != SUCCESS)
		{
			status = -1;
		}

		if(status == 0 && relative_path[0] != '\0')
		{
			if(m_concat_literal(destination_path_out,"/") != SUCCESS)
			{
				status = -1;
			}
		}

		if(status == 0 && relative_path[0] != '\0')
		{
			if(m_concat_string(destination_path_out,relative_path) != SUCCESS)
			{
				status = -1;
			}
		}
	}

	return(status);
}

/**
 * @brief Create one directory and accept an existing path that resolves to a directory
 *
 * Existing symlinks to directories are treated as valid path components
 *
 * @param[in] directory_path Absolute directory path
 *
 * @return 0 on success, non-zero on failure
 */
static int create_directory_if_missing(const char *directory_path)
{
	int status = 0;
	struct stat directory_stat = {0};

	if(directory_path == NULL)
	{
		status = -1;
	}

	if(status == 0 && mkdir(directory_path,0700) != 0 && errno != EEXIST)
	{
		status = -1;
	}

	if(status == 0 && stat(directory_path,&directory_stat) != 0)
	{
		status = -1;
	}

	if(status == 0 && !S_ISDIR(directory_stat.st_mode))
	{
		status = -1;
	}

	return(status);
}

/**
 * @brief Construct absolute path from environment root and relative child path
 *
 * @param[in] environment_variable_name Environment variable containing root path
 * @param[in] relative_path Relative child path to append
 * @param[out] absolute_path_out Output absolute path buffer
 *
 * @return Return status code
 */
static Return construct_path_from_environment_variable(
	const char *environment_variable_name,
	const char *relative_path,
	memory     *absolute_path_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	const char *root_path = NULL;
	size_t absolute_path_size = 0U;

	if(environment_variable_name == NULL || relative_path == NULL || absolute_path_out == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		root_path = getenv(environment_variable_name);

		if(root_path == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		absolute_path_size = strlen(root_path) + strlen(relative_path) + 2U;
		status = m_resize(absolute_path_out,absolute_path_size);
	}

	if(SUCCESS == status)
	{
		char *absolute_path_data = m_data(char,absolute_path_out);

		if(absolute_path_data == NULL)
		{
			status = FAILURE;
		} else {
			int print_result = snprintf(absolute_path_data,absolute_path_size,"%s/%s",root_path,relative_path);

			if(print_result < 0)
			{
				status = FAILURE;
			} else {
				status = m_finalize_string(absolute_path_out,(size_t)print_result);
			}
		}
	}

	return(status);
}

/**
 * @brief Copy regular file contents into destination path
 *
 * @param[in] source_path Source regular file path
 * @param[in] destination_path Destination regular file path
 * @param[in] destination_mode Mode for destination file
 *
 * @return 0 on success, non-zero on failure
 */
static int copy_regular_file_contents(
	const char   *source_path,
	const char   *destination_path,
	const mode_t destination_mode)
{
	int status = 0;
	int source_fd = -1;
	int destination_fd = -1;
	unsigned char buffer[65536];

	if(source_path == NULL || destination_path == NULL)
	{
		status = -1;
	}

	if(status == 0)
	{
		source_fd = open(source_path,O_RDONLY);

		if(source_fd < 0)
		{
			status = -1;
		}
	}

	if(status == 0)
	{
		destination_fd = open(destination_path,
			O_WRONLY | O_CREAT | O_TRUNC,
			destination_mode & (mode_t)07777);

		if(destination_fd < 0)
		{
			status = -1;
		}
	}

	if(status == 0 && fchmod(destination_fd,destination_mode & (mode_t)07777) != 0)
	{
		status = -1;
	}

	while(status == 0)
	{
		const ssize_t bytes_read = read(source_fd,buffer,sizeof(buffer));

		if(bytes_read < 0)
		{
			if(errno == EINTR)
			{
				continue;
			}
			status = -1;
			break;
		}

		if(bytes_read == 0)
		{
			break;
		}

		ssize_t offset = 0;

		while(offset < bytes_read)
		{
			const ssize_t bytes_written = write(destination_fd,
				buffer + offset,
				(size_t)(bytes_read - offset));

			if(bytes_written < 0)
			{
				if(errno == EINTR)
				{
					continue;
				}
				status = -1;
				break;
			}

			offset += bytes_written;
		}
	}

	if(source_fd >= 0 && close(source_fd) != 0)
	{
		status = -1;
	}

	if(destination_fd >= 0 && close(destination_fd) != 0)
	{
		status = -1;
	}

	return(status);
}

/**
 * @brief Copy symbolic link target from source to destination
 *
 * @param[in] source_path Source symbolic link path
 * @param[in] destination_path Destination symbolic link path
 *
 * @return 0 on success, non-zero on failure
 */
static int copy_symbolic_link(
	const char *source_path,
	const char *destination_path)
{
	int status = 0;
	size_t link_target_buffer_size = 256U;
	char *link_target = NULL;

	if(source_path == NULL || destination_path == NULL)
	{
		status = -1;
	}

	if(status == 0)
	{
		link_target = malloc(link_target_buffer_size);

		if(link_target == NULL)
		{
			status = -1;
		}
	}

	while(status == 0)
	{
		ssize_t link_target_size = readlink(source_path,link_target,link_target_buffer_size - 1U);

		if(link_target_size < 0)
		{
			status = -1;
			break;
		}

		if((size_t)link_target_size < link_target_buffer_size - 1U)
		{
			link_target[link_target_size] = '\0';
			break;
		}

		link_target_buffer_size *= 2U;
		char *resized_link_target = realloc(link_target,link_target_buffer_size);

		if(resized_link_target == NULL)
		{
			status = -1;
		} else {
			link_target = resized_link_target;
		}
	}

	if(status == 0 && symlink(link_target,destination_path) != 0)
	{
		status = -1;
	}

	free(link_target);

	return(status);
}

/**
 * @brief Copy callback used by nftw
 *
 * @param[in] path Current path from nftw walk
 * @param[in] stat_buffer File stat pointer from nftw
 * @param[in] type_flag nftw node type
 * @param[in] ftw_buffer Unused nftw frame data
 *
 * @return 0 on success, non-zero on failure
 */
static int nftw_copy_callback(
	const char        *path,
	const struct stat *stat_buffer,
	const int         type_flag,
	struct FTW        *ftw_buffer)
{
	(void)ftw_buffer;

	int status = 0;
	m_create(char,destination_path,MEMORY_STRING);
	const char *destination_path_string = NULL;

	if(stat_buffer == NULL)
	{
		status = -1;
	}

	if(status == 0 && build_copy_destination_path_for_nftw(path,destination_path) != 0)
	{
		status = -1;
	}

	if(status == 0)
	{
		destination_path_string = m_text(destination_path);

		if(destination_path_string == NULL)
		{
			status = -1;
		}
	}

	if(status == 0)
	{
		switch(type_flag)
		{
			case FTW_D:
				if(mkdir(destination_path_string,stat_buffer->st_mode & (mode_t)07777) != 0 && errno != EEXIST)
				{
					status = -1;
				}

				if(status == 0 && chmod(destination_path_string,stat_buffer->st_mode & (mode_t)07777) != 0)
				{
					status = -1;
				}
				break;
			case FTW_F:
				if(copy_regular_file_contents(path,destination_path_string,stat_buffer->st_mode) != 0)
				{
					status = -1;
				}

				if(status == 0 && apply_mtime_from_source_stat(destination_path_string,stat_buffer,0) != 0)
				{
					status = -1;
				}
				break;
			case FTW_SL:
#ifdef FTW_SLN
			case FTW_SLN:
#endif

				if(unlink(destination_path_string) != 0 && errno != ENOENT)
				{
					status = -1;
				}

				if(status == 0 && copy_symbolic_link(path,destination_path_string) != 0)
				{
					status = -1;
				}
#ifdef AT_SYMLINK_NOFOLLOW
				if(status == 0
				        && apply_mtime_from_source_stat(destination_path_string,stat_buffer,AT_SYMLINK_NOFOLLOW) != 0)
				{
					status = -1;
				}
#endif
				break;
			default:
				status = -1;
				break;
		}
	}

	m_del(destination_path);

	return(status);
}

/**
 * @brief Post-order callback to restore copied directory mtimes
 *
 * @param[in] path Current source path from nftw walk
 * @param[in] stat_buffer Source stat metadata
 * @param[in] type_flag nftw node type
 * @param[in] ftw_buffer Unused nftw frame data
 *
 * @return 0 on success, non-zero on failure
 */
static int nftw_sync_directory_mtime_callback(
	const char        *path,
	const struct stat *stat_buffer,
	const int         type_flag,
	struct FTW        *ftw_buffer)
{
	(void)ftw_buffer;

	int status = 0;
	m_create(char,destination_path,MEMORY_STRING);
	const char *destination_path_string = NULL;
	bool is_directory = false;

	if(stat_buffer == NULL)
	{
		status = -1;
	}

	if(type_flag == FTW_D)
	{
		is_directory = true;
	}
#ifdef FTW_DP
	if(type_flag == FTW_DP)
	{
		is_directory = true;
	}
#endif

	if(status == 0 && is_directory == true)
	{
		if(build_copy_destination_path_for_nftw(path,destination_path) != 0)
		{
			status = -1;
		}
	}

	if(status == 0 && is_directory == true)
	{
		destination_path_string = m_text(destination_path);

		if(destination_path_string == NULL)
		{
			status = -1;
		}
	}

	if(status == 0 && is_directory == true)
	{
		if(apply_mtime_from_source_stat(destination_path_string,stat_buffer,0) != 0)
		{
			status = -1;
		}
	}

	m_del(destination_path);

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
 * @brief Create directory tree by path relative to TMPDIR
 *
 * Empty path resolves to TMPDIR root
 * Existing directories are preserved
 * Existing symlinks to directories in the TMPDIR prefix are accepted
 *
 * @param[in] relative_path_to_tmpdir Directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Directory tree exists after the call
 *         - FAILURE: Path construction or directory creation failed
 */
Return create_directory(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	char *absolute_path_data = NULL;
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
		absolute_path_data = m_data(char,absolute_path);

		if(absolute_path_data == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		size_t absolute_path_length = strlen(absolute_path_data);

		while(absolute_path_length > 1U && absolute_path_data[absolute_path_length - 1U] == '/')
		{
			absolute_path_data[absolute_path_length - 1U] = '\0';
			absolute_path_length--;
		}
	}

	if(SUCCESS == status)
	{
		for(char *path_cursor = absolute_path_data + 1;
		        SUCCESS == status && *path_cursor != '\0';
		        path_cursor++)
		{
			if(*path_cursor == '/')
			{
				*path_cursor = '\0';

				if(create_directory_if_missing(absolute_path_data) != 0)
				{
					status = FAILURE;
				}
				*path_cursor = '/';
			}
		}
	}

	if(SUCCESS == status)
	{
		if(create_directory_if_missing(absolute_path_data) != 0)
		{
			status = FAILURE;
		}
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Remove file or directory tree by path relative to TMPDIR
 *
 * When relative_path_to_tmpdir is an empty string, the function targets TMPDIR itself
 * Missing paths are treated as a hard failure to keep test cleanup strict and deterministic
 *
 * @param[in] relative_path_to_tmpdir File or directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Path was removed
 *         - FAILURE: Path construction or remove traversal failed
 */
Return delete_path(const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat path_stat = {0};
	m_create(char,absolute_path,MEMORY_STRING);

	if(relative_path_to_tmpdir == NULL)
	{
		echo(STDERR,"delete_path: relative path must not be NULL\n");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_path_to_tmpdir,absolute_path);

		if(SUCCESS != status)
		{
			echo(STDERR,"delete_path: failed to construct absolute path for \"%s\"\n",relative_path_to_tmpdir);
		}
	}

	if(SUCCESS == status)
	{
		if(lstat(m_text(absolute_path),&path_stat) != 0)
		{
			echo(STDERR,"delete_path: lstat failed for %s: %s\n",m_text(absolute_path),strerror(errno));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(S_ISDIR(path_stat.st_mode))
		{
			long sc_open_max = sysconf(_SC_OPEN_MAX);

			if(sc_open_max <= 0)
			{
				sc_open_max = 512;
			}

			if(nftw(m_text(absolute_path),nftw_remove_callback,(int)sc_open_max,FTW_DEPTH | FTW_PHYS) != 0)
			{
				echo(STDERR,"delete_path: nftw failed for %s: %s\n",m_text(absolute_path),strerror(errno));
				status = FAILURE;
			}
		} else if(remove(m_text(absolute_path)) != 0){
			echo(STDERR,"delete_path: remove failed for %s: %s\n",m_text(absolute_path),strerror(errno));
			status = FAILURE;
		}
	}

	m_del(absolute_path);

	return(status);
}

/**
 * @brief Copy filesystem object from one absolute path to another
 *
 * @param[in] source_absolute_path Source absolute path
 * @param[in] destination_absolute_path Destination absolute path
 * @param[in] flags Behavior flags such as @ref REQUIRE_SOURCE_EXISTS or @ref ALLOW_MISSING_SOURCE
 *
 * @return Return status code
 */
static Return copy_absolute_path(
	const char   *source_absolute_path,
	const char   *destination_absolute_path,
	unsigned int flags)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	struct stat source_stat = {0};
	struct stat destination_stat = {0};
	bool destination_exists = false;

	if(source_absolute_path == NULL || destination_absolute_path == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && lstat(source_absolute_path,&source_stat) != 0)
	{
		if(errno == ENOENT && (flags & ALLOW_MISSING_SOURCE) != 0U)
		{
			return(SUCCESS);
		}

		status = FAILURE;
	}

	if(SUCCESS == status && S_ISDIR(source_stat.st_mode))
	{
		size_t source_length = strlen(source_absolute_path);

		while(source_length > 1U && source_absolute_path[source_length - 1U] == '/')
		{
			source_length--;
		}

		if(strncmp(destination_absolute_path,source_absolute_path,source_length) == 0
		        && (destination_absolute_path[source_length] == '\0'
		        || destination_absolute_path[source_length] == '/'))
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(lstat(destination_absolute_path,&destination_stat) == 0)
		{
			destination_exists = true;
		} else if(errno != ENOENT){
			status = FAILURE;
		}
	}

	if(SUCCESS == status && S_ISDIR(source_stat.st_mode))
	{
		if(destination_exists == true)
		{
			status = FAILURE;
		} else {
			copy_source_root_for_nftw = source_absolute_path;
			copy_destination_root_for_nftw = destination_absolute_path;

			if(copy_source_root_for_nftw == NULL
			        || copy_destination_root_for_nftw == NULL)
			{
				status = FAILURE;
			}
		}
	}

	if(SUCCESS == status && S_ISDIR(source_stat.st_mode) && destination_exists == false)
	{
		copy_source_root_length_for_nftw = strlen(copy_source_root_for_nftw);
		copy_destination_root_length_for_nftw = strlen(copy_destination_root_for_nftw);

		if(copy_source_root_length_for_nftw == 0U
		        || copy_destination_root_length_for_nftw == 0U)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && S_ISDIR(source_stat.st_mode))
	{
		if(destination_exists == false)
		{
			if(nftw(source_absolute_path,nftw_copy_callback,64,FTW_PHYS) != 0)
			{
				status = FAILURE;
			}

			if(SUCCESS == status
			        && nftw(source_absolute_path,
				nftw_sync_directory_mtime_callback,
				64,
				FTW_DEPTH | FTW_PHYS) != 0)
			{
				status = FAILURE;
			}
		}
	} else if(SUCCESS == status && S_ISREG(source_stat.st_mode)){
		if(destination_exists == true && S_ISDIR(destination_stat.st_mode))
		{
			status = FAILURE;
		}

		if(SUCCESS == status)
		{
			if(unlink(destination_absolute_path) != 0 && errno != ENOENT)
			{
				status = FAILURE;
			}
		}

		if(SUCCESS == status
		        && copy_regular_file_contents(source_absolute_path,
			destination_absolute_path,
			source_stat.st_mode) != 0)
		{
			status = FAILURE;
		}

		if(SUCCESS == status
		        && apply_mtime_from_source_stat(destination_absolute_path,&source_stat,0) != 0)
		{
			status = FAILURE;
		}
	} else if(SUCCESS == status && S_ISLNK(source_stat.st_mode)){
		if(destination_exists == true && S_ISDIR(destination_stat.st_mode))
		{
			status = FAILURE;
		}

		if(SUCCESS == status && destination_exists == true
		        && unlink(destination_absolute_path) != 0)
		{
			status = FAILURE;
		}

		if(SUCCESS == status && copy_symbolic_link(source_absolute_path,destination_absolute_path) != 0)
		{
			status = FAILURE;
		}

#ifdef AT_SYMLINK_NOFOLLOW
		if(SUCCESS == status
		        && apply_mtime_from_source_stat(destination_absolute_path,
			&source_stat,
			AT_SYMLINK_NOFOLLOW) != 0)
		{
			status = FAILURE;
		}
#endif
	} else if(SUCCESS == status){
		status = FAILURE;
	}

	reset_nftw_copy_context();

	return(status);
}

/**
 * @brief Copy file or directory tree by path relative to TMPDIR
 *
 * Empty source or destination path resolves to TMPDIR root
 * Directory copy into itself or into its own subtree is rejected
 *
 * @param[in] relative_source_path Source file or directory path relative to TMPDIR
 * @param[in] relative_destination_path Destination file or directory path relative to TMPDIR
 *
 * @return Return status code:
 *         - SUCCESS: Source path was copied to destination path
 *         - FAILURE: Validation, path resolution, stat lookup, or copy operation failed
 */
Return copy_path(
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

	if(SUCCESS == status)
	{
		status = copy_absolute_path(m_text(source_absolute_path),
			m_text(destination_absolute_path),
			REQUIRE_SOURCE_EXISTS);
	}
	m_del(destination_absolute_path);
	m_del(source_absolute_path);

	return(status);
}

/**
 * @brief Copy file or directory tree from ORIGIN_DIR into TMPDIR
 *
 * @param[in] relative_source_path_to_origin_dir Source path relative to ORIGIN_DIR
 * @param[in] relative_destination_path_to_tmpdir Destination path relative to TMPDIR
 * @param[in] flags Behavior flags such as @ref REQUIRE_SOURCE_EXISTS or @ref ALLOW_MISSING_SOURCE
 *
 * @return Return status code:
 *         - SUCCESS: Source path was copied or was absent and allowed to be missing
 *         - FAILURE: Validation, path resolution, stat lookup, or copy operation failed
 */
Return copy_from_origin(
	const char   *relative_source_path_to_origin_dir,
	const char   *relative_destination_path_to_tmpdir,
	unsigned int flags)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	m_create(char,source_absolute_path,MEMORY_STRING);
	m_create(char,destination_absolute_path,MEMORY_STRING);

	if(relative_source_path_to_origin_dir == NULL || relative_destination_path_to_tmpdir == NULL)
	{
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		status = construct_path_from_environment_variable("ORIGIN_DIR",
			relative_source_path_to_origin_dir,
			source_absolute_path);
	}

	if(SUCCESS == status)
	{
		status = construct_path(relative_destination_path_to_tmpdir,destination_absolute_path);
	}

	if(SUCCESS == status)
	{
		status = copy_absolute_path(m_text(source_absolute_path),
			m_text(destination_absolute_path),
			flags);
	}

	m_del(destination_absolute_path);
	m_del(source_absolute_path);

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
 * @brief Compute SHA512 for a file using the project SHA512 library
 *
 * @param[in] file_path File path passed directly to `fopen()`
 * @param[out] sha512_out Output SHA512 digest buffer
 *
 * @return Return status code:
 *         - SUCCESS: SHA512 digest computed successfully
 *         - FAILURE: Validation, I/O, or hash operation failed
 */
Return compute_file_sha512(
	const char    *file_path,
	unsigned char *sha512_out)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	unsigned char buffer[65536];
	SHA512_Context context = {0};

	if(file_path == NULL || sha512_out == NULL)
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

	if(SUCCESS == status && sha512_init(&context) != CRYPT_OK)
	{
		status = FAILURE;
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

		if(sha512_update(&context,buffer,bytes_read) != CRYPT_OK)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && sha512_final(&context,sha512_out) != CRYPT_OK)
	{
		status = FAILURE;
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
