#include "precizer.h"
#include <errno.h>

/**
 * @brief Calculate SHA512 cryptographic hash of a file, optionally resuming from offset.
 *
 * Reads file data starting from @p offset, updates @p mdContext, increments
 * @p summary->total_hashed_bytes for each processed chunk, and accumulates
 * per-call hashing elapsed time into @p summary->total_hashing_elapsed_ns.
 *
 * @param path File path (relative or absolute).
 * @param path_size Length of @p path.
 * @param file_buffer Read buffer descriptor.
 * @param sha512 Output digest buffer (written after finalization).
 * @param offset In/out byte offset for resume/interruption handling.
 * @param summary Traversal counters updated with hashed byte count and hashing time.
 * @param mdContext SHA512 context for incremental hashing.
 * @param read_error Output flag set when reading fails.
 * @param read_errno Output errno snapshot for read errors.
 * @param wrong_file_type Output flag for non-seekable/special files.
 * @return SUCCESS or FAILURE.
 */
Return sha512sum(
	const char       *path,
	const size_t     path_size,
	memory           *file_buffer,
	unsigned char    *sha512,
	sqlite3_int64    *offset,
	TraversalSummary *summary,
	SHA512_Context   *mdContext,
	bool             *read_error,
	int              *read_errno,
	bool             *wrong_file_type)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	if(file_buffer->length == 0)
	{
		slog(ERROR,"Invalid buffer size: %ld bytes\n",file_buffer->length);
		provide(FAILURE);
	}

	char *absolute_path = NULL;

	FILE *fileptr = fopen(path,"rb");

	if(fileptr == NULL)
	{
		// No read permission
		if(errno == EACCES)
		{
			*read_error = true;

			*read_errno = errno;

			provide(status);
		}

		status = path_absolute_from_relative(&absolute_path,path,path_size);

		if(absolute_path == NULL || SUCCESS != status)
		{
			slog(ERROR,"Can't constructs an absolute path from the base directory %s and a relative path %s\n",config->running_dir,path);

			if(absolute_path != NULL)
			{
				free(absolute_path);
			}
			provide(status);
		}

		fileptr = fopen(absolute_path,"rb");

		if(fileptr == NULL)
		{
			// No read permission
			if(errno == EACCES)
			{
				*read_error = true;

				*read_errno = errno;

				free(absolute_path);
				provide(status);
			}

			slog(ERROR,"Can open the file using neither relative %s nor absolute %s path with errno: %d\n",path,absolute_path,errno);
			free(absolute_path);
			provide(FAILURE);
		}
	}

	// It moves the file pointer "offset" bytes from the beginning of the file
	if(fseek(fileptr,*offset,SEEK_SET) != 0)
	{
		/* Looks like the wrong file type.
		   Doesn't need to return FAILURE status */
		*wrong_file_type = true;
		free(absolute_path);
		fclose(fileptr);
		provide(status);
	}

	bool loop_was_interrupted = false;
	bool perform_file_hashing = config->dry_run == false
	        || config->dry_run_with_checksums == true;

	if(*offset == 0)
	{
		if(sha512_init(mdContext) == 1)
		{
			slog(ERROR,"SHA512 initialization failed\n");
			free(absolute_path);
			fclose(fileptr);
			provide(FAILURE);
		}
	}

	if(perform_file_hashing == true)
	{
		long long int hashing_start_ns = cur_time_monotonic_ns();

		unsigned char *buffer = rawdata(file_buffer);

		while(true)
		{
			/* Interrupt the loop smoothly */
			/* Interrupt when Ctrl+C */
			if(global_interrupt_flag == true)
			{
				loop_was_interrupted = true;
				break;
			}

			size_t len = fread(buffer,sizeof(unsigned char),file_buffer->length,fileptr);

			if(len == 0)
			{
				if(ferror(fileptr))
				{
					*read_error = true;
					*read_errno = errno;
				}

				break;
			}

			if(SUCCESS == status)
			{
				if(sha512_update(mdContext,buffer,len) == 1)
				{
					slog(ERROR,"SHA512 update failed\n");
					status = FAILURE;
					break;
				}

				*offset += (sqlite3_int64)len;
				summary->total_hashed_bytes += len;
			}
		}

		long long int hashing_stop_ns = cur_time_monotonic_ns();

		long long int hashing_elapsed_ns = hashing_stop_ns - hashing_start_ns;

		if(hashing_elapsed_ns < 0LL)
		{
			hashing_elapsed_ns = 0LL;
		}

		summary->total_hashing_elapsed_ns += hashing_elapsed_ns;
	}

	if(fclose(fileptr) != 0)
	{
		slog(ERROR,"Error closing file %s\n",path);
	}

	free(absolute_path);

	if(SUCCESS == status
	        && perform_file_hashing == true
	        && loop_was_interrupted == false)
	{
		*offset = 0;

		if(sha512_final(mdContext,sha512) == 1)
		{
			slog(ERROR,"SHA512 finalization failed\n");
			status = FAILURE;
		}
	}

#if 0

	for(size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		printf("%02x",sha512[i]);
	}
	putchar('\n');

#endif

	provide(status);
}
