#include "sute.h"

/**
 * @brief Truncate an existing file to zero bytes by reopening it in binary write mode
 *
 * @param[in] relative_path_to_tmpdir Relative path from TMPDIR to the target file
 *
 * @return Return status code:
 *         - SUCCESS: File was truncated successfully
 *         - FAILURE: Path construction or file operation failed
 */
Return truncate_file_to_zero_size(
	const char *relative_path_to_tmpdir)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	create(char,absolute_path);

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
		FILE *file = fopen(getcstring(absolute_path),"wb");

		if(file == NULL)
		{
			status = FAILURE;
		} else if(fclose(file) != 0){
			status = FAILURE;
		}
	}

	del(absolute_path);

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
	create(char,absolute_path);

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
		status = get_file_stat(getcstring(absolute_path),&before_stat);
	}

	if(SUCCESS == status)
	{
		const off_t grown_size = before_stat.st_size + (off_t)131072;

		// Grow logical size via truncate to create a sparse tail without writing payload bytes
		if(grown_size <= before_stat.st_size)
		{
			status = FAILURE;
		} else if(truncate(getcstring(absolute_path),grown_size) != 0){
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(getcstring(absolute_path),&after_stat);
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

	del(absolute_path);

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
 *         - SUCCESS: Dense rewrite completed with unchanged logical size and changed allocated blocks
 *         - FAILURE: Validation or filesystem operation failed
 */
Return rewrite_file_dense_with_same_size(
	const char   *relative_path_to_tmpdir,
	const off_t  target_size,
	const blkcnt_t blocks_before_rewrite)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	FILE *file = NULL;
	struct stat after_stat = {0};
	unsigned char buffer[4096];
	create(char,absolute_path);

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
		file = fopen(getcstring(absolute_path),"wb");

		if(file == NULL)
		{
			status = FAILURE;
		}
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

	if(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		status = get_file_stat(getcstring(absolute_path),&after_stat);
	}

	if(SUCCESS == status && after_stat.st_size != target_size)
	{
		status = FAILURE;
	}

	if(SUCCESS == status && after_stat.st_blocks == blocks_before_rewrite)
	{
		status = FAILURE;
	}

	del(absolute_path);

	return(status);
}

/**
 * @brief Compute SHA512 for a file using the project SHA512 library
 *
 * @param[in] file_path Path to the file to hash
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

	if(SUCCESS == status && sha512_init(&context) == 1)
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

		if(sha512_update(&context,buffer,bytes_read) == 1)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status && sha512_final(&context,sha512_out) == 1)
	{
		status = FAILURE;
	}

	if(file != NULL)
	{
		(void)fclose(file);
	}

	return(status);
}

/**
 * @brief Append one byte to a file using native C file I/O
 *
 * @param[in] file_path Path to the file to append
 * @param[in] byte Byte value to append
 *
 * @return Return status code:
 *         - SUCCESS: Byte appended successfully
 *         - FAILURE: Validation or I/O failed
 */
Return append_byte_to_file(
	const char   *file_path,
	unsigned char byte)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(file_path == NULL)
	{
		status = FAILURE;
	}

	FILE *file = NULL;

	if(SUCCESS == status)
	{
		file = fopen(file_path,"ab");
		if(file == NULL)
		{
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		if(fwrite(&byte,sizeof(unsigned char),1U,file) != 1U)
		{
			status = FAILURE;
		}
	}

	if(file != NULL)
	{
		if(fclose(file) != 0)
		{
			status = FAILURE;
		}
	}

	return(status);
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
	create(char,source_absolute_path);
	create(char,target_absolute_path);

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
		status = get_file_stat(getcstring(source_absolute_path),&source_file_stat);
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
		if(utimensat(0,getcstring(target_absolute_path),target_times,0) != 0)
		{
			status = FAILURE;
		}
	}

	del(source_absolute_path);
	del(target_absolute_path);

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
Return tamper_locked_file_bytes(
	const char *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;
	int fd = -1;
	struct stat before = {0};
	unsigned char buffer[2] = {0};
	struct timespec times[2] = {{0}};
	create(char,file_path);

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
	if(SUCCESS == status && (fd = open(getcstring(file_path),O_RDWR)) < 0)
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

	del(file_path);

	return(status);
}
