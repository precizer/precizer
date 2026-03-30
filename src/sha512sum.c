#include "precizer.h"
#include <errno.h>

#ifdef TESTITALL_TEST_HOOKS
/**
 * @brief Check whether a path points to the large interruption test file.
 */
static bool is_huge_interruption_target(const char *path)
{
	if(path == NULL)
	{
		return(false);
	}

	const char *needle = "hugetestfile";
	const size_t path_length = strlen(path);
	const size_t needle_length = strlen(needle);

	if(path_length < needle_length)
	{
		return(false);
	}

	return(0 == strcmp(path + (path_length - needle_length),needle));
}

/**
 * @brief Generate a pseudo-random stop byte in the closed range [1, file_size].
 */
static uint64_t random_stop_byte(const uint64_t file_size)
{
	if(file_size == 0U)
	{
		return(0U);
	}

	struct timespec now = {0};
	(void)clock_gettime(CLOCK_MONOTONIC,&now);

	uint64_t seed = (uint64_t)now.tv_nsec;
	seed ^= ((uint64_t)now.tv_sec << 32);
	seed ^= (uint64_t)getpid();

	return((seed % file_size) + 1U);
}
#endif

/**
 * @brief Calculate SHA512 cryptographic hash of a file, optionally resuming from offset.
 *
 * Reads file data starting from @p file->checksum_offset, updates @p file->mdContext,
 * increments @p summary->total_hashed_bytes for each processed chunk, and accumulates
 * per-call hashing elapsed time into @p summary->total_hashing_elapsed_ns.
 *
 * @param path File path (relative or absolute).
 * @param path_size Length of @p path.
 * @param file_buffer Read buffer descriptor.
 * @param summary Traversal counters updated with hashed byte count and hashing time.
 * @param file Per-file state object used as input and output. checksum_offset is the
 *             starting byte offset for resumption and is updated as bytes are
 *             hashed. sha512 receives the final digest. mdContext holds the
 *             incremental hashing state. read_error and read_errno describe
 *             read failures. wrong_file_type is set for non-seekable or
 *             otherwise unsupported file types
 * @return SUCCESS or FAILURE.
 */
Return sha512sum(
	const char       *path,
	const size_t     path_size,
	memory           *file_buffer,
	TraversalSummary *summary,
	File             *file)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
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
			// Flag the read failure
			file->read_error = true;

			// Preserve errno before returning
			file->read_errno = errno;

			provide(status);
		}

		status = path_absolute_from_relative(&absolute_path,path,path_size);

		if(absolute_path == NULL || (TRIUMPH & status) == 0)
		{
			slog(ERROR,"Can't constructs an absolute path from the base directory %s and a relative path %s\n",confstr(running_dir),path);

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
				file->read_error = true;

				file->read_errno = errno;

				free(absolute_path);
				provide(status);
			}

			slog(ERROR,"Can open the file using neither relative %s nor absolute %s path with errno: %d\n",path,absolute_path,errno);
			free(absolute_path);
			provide(FAILURE);
		}
	}

	// Move the file pointer checksum_offset bytes from the beginning of the file
	if(fseek(fileptr,file->checksum_offset,SEEK_SET) != 0)
	{
		/*
		 * This looks like an unsupported file type.
		 * Doesn't need to return FAILURE status.
		 */
		file->wrong_file_type = true;
		free(absolute_path);
		fclose(fileptr);
		provide(status);
	}

	bool loop_was_interrupted = false;
	bool perform_file_hashing = config->dry_run == false
	        || config->dry_run_with_checksums == true;

#ifdef TESTITALL_TEST_HOOKS
	/*
	 * 0 means random-stop flow is disabled for this file.
	 * Non-zero means upper bound for random stop byte selection.
	 */
	uint64_t random_stop_limit = 0U;
	/* 0 means stop byte has not been selected yet. */
	uint64_t random_stop_byte_value = 0U;
	/* Separate state flag: do not overload stop-byte numeric value. */
	bool random_stop_triggered = false;
#endif

	if(file->checksum_offset == 0)
	{
		// Fresh hashing pass: initialize the SHA512 state from scratch
		if(sha512_init(&file->mdContext) == 1)
		{
			slog(ERROR,"SHA512 initialization failed\n");
			free(absolute_path);
			fclose(fileptr);
			provide(FAILURE);
		}
	}

#ifdef TESTITALL_TEST_HOOKS
	/*
	 * Activate random interruption only for a fresh pass of hugetestfile.
	 * Resume path (checksum_offset > 0 at entry) must continue from saved state
	 * without selecting a new random stop point.
	 */
	if(file->checksum_offset == 0
	        && is_huge_interruption_target(path) == true
	        && file->stat.st_size > 0)
	{
		random_stop_limit = (uint64_t)file->stat.st_size;

		/*
		 * Select interruption target before the first fread() call so
		 * the first chunk can be bounded and the stop point can land
		 * anywhere in [1, file_size].
		 */
		random_stop_byte_value = random_stop_byte(random_stop_limit);

		/* Defensive fallback: never allow a zero stop byte. */
		if(random_stop_byte_value == 0U)
		{
			random_stop_byte_value = 1U;
		}

		/*
		 * Keep the stop point strictly inside file data for multi-byte files.
		 * If random selection lands exactly at EOF, shift it one byte left.
		 * The guard keeps subtraction safe and avoids unsigned underflow.
		 */
		if(random_stop_limit > 1U && random_stop_byte_value >= random_stop_limit)
		{
			random_stop_byte_value = random_stop_limit - 1U;
		}
	}
#endif

	if(perform_file_hashing == true)
	{
		long long int hashing_start_ns = cur_time_monotonic_ns();

		unsigned char *buffer = rawdata(file_buffer);

		while(true)
		{
#ifdef TESTITALL_TEST_HOOKS
			/*
			 * Trigger point 2 exactly once when selected stop byte is reached.
			 */
			if(random_stop_limit > 0U
			        && random_stop_triggered == false
			        && random_stop_byte_value > 0U
			        && (uint64_t)(file->checksum_offset) >= random_stop_byte_value)
			{
				signal_wait_at_point(2U);
				random_stop_triggered = true;
			}
#endif

			/* Interrupt the loop smoothly */
			/* Interrupt when Ctrl+C */
#ifdef TESTITALL_TEST_HOOKS
			/*
			 * Temporary guard: when random-stop mode is active for hugetestfile,
			 * do not break on global_interrupt_flag until point 2 has really
			 * happened. Otherwise interruption may fire too early and miss the
			 * controlled "interrupt at random byte" scenario.
			 */
			bool delay_interrupt_for_random_stop = false;

			if(random_stop_limit > 0U && random_stop_triggered == false)
			{
				if(random_stop_byte_value == 0U)
				{
					/* No stop byte yet: wait until at least one block is hashed. */
					delay_interrupt_for_random_stop = true;

				} else if((uint64_t)(file->checksum_offset) < random_stop_byte_value){
					/* Stop byte is known but not reached yet: keep hashing. */
					delay_interrupt_for_random_stop = true;
				}
			}
#endif

			if(global_interrupt_flag == true
#ifdef TESTITALL_TEST_HOOKS
			        && delay_interrupt_for_random_stop == false
#endif
			)
			{
				loop_was_interrupted = true;
				break;
			}

			size_t read_limit = file_buffer->length;

#ifdef TESTITALL_TEST_HOOKS
			/*
			 * Keep the read bounded so offset can stop exactly at the selected
			 * byte instead of jumping to EOF in a single large fread().
			 */
			if(random_stop_limit > 0U
			        && random_stop_triggered == false
			        && random_stop_byte_value > 0U
			        && (uint64_t)(file->checksum_offset) < random_stop_byte_value)
			{
				const uint64_t bytes_left_to_stop = random_stop_byte_value - (uint64_t)(file->checksum_offset);

				if(bytes_left_to_stop < (uint64_t)read_limit)
				{
					read_limit = (size_t)bytes_left_to_stop;
				}
			}
#endif

			size_t len = fread(buffer,sizeof(unsigned char),read_limit,fileptr);

			if(len == 0)
			{
				if(ferror(fileptr))
				{
					file->read_error = true;
					file->read_errno = errno;
				}

				break;
			}

			if(SUCCESS == status)
			{
				if(sha512_update(&file->mdContext,buffer,len) == 1)
				{
					slog(ERROR,"SHA512 update failed\n");
					status = FAILURE;
					break;
				}

				file->checksum_offset += (sqlite3_int64)len;
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
		file->checksum_offset = 0;

		if(sha512_final(&file->mdContext,file->sha512) == 1)
		{
			slog(ERROR,"SHA512 finalization failed\n");
			status = FAILURE;
		}
	}

#if 0
	for(size_t i = 0; i < SHA512_DIGEST_LENGTH; i++)
	{
		printf("%02x",file->sha512[i]);
	}
	putchar('\n');
#endif

	provide(status);
}
