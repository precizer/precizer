#include "testitall.h"
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>

static const char *copy_source_root_for_nftw = NULL;
static const char *copy_destination_root_for_nftw = NULL;
static size_t copy_source_root_length_for_nftw = 0U;
static size_t copy_destination_root_length_for_nftw = 0U;

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
 * Empty source or destination path resolves to TMPDIR root. The destination
 * is an exact target path and must not be an existing directory. Directory
 * copy into itself or into its own subtree is rejected
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
 * Empty source or destination path resolves to the corresponding environment
 * root. Missing sources fail with REQUIRE_SOURCE_EXISTS and are accepted with
 * ALLOW_MISSING_SOURCE
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
