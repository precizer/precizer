#include "precizer.h"
#include <errno.h>
#include <fcntl.h>

/**
 * @brief Read a file and update its SHA512 state when hashing is enabled
 *
 * Opens @p relative_path from @p root_directory_fd. When normal hashing is
 * enabled, or when dry-run uses `--dry-run=with-checksums`, the function reads
 * file data starting from @p file->checksum_offset, updates @p file->mdContext,
 * counts hashed bytes in @p summary, and finalizes @p file->sha512 after an
 * uninterrupted pass. In dry-run mode without checksum calculation, the file is
 * opened and seek-checked but the checksum state is not advanced
 *
 * File opening problems are stored in @p file as read errors and do not turn
 * into a function failure. Technical problems that prevent safe hashing still
 * return FAILURE
 *
 * @param root_directory_fd Open traversal root descriptor used as the path base
 * @param relative_path File path relative to @p root_directory_fd
 * @param file_buffer Read buffer descriptor
 * @param summary Traversal counters updated with hashed byte count and hashing time
 * @param file Per-file state object used as input and output. checksum_offset is the
 *             starting byte offset for resumption and is updated as bytes are
 *             hashed. sha512 receives the final digest. mdContext holds the
 *             incremental hashing state. read_error and read_errno describe
 *             read failures. wrong_file_type is set for non-seekable or
 *             otherwise unsupported file types
 * @return SUCCESS when the file was handled cleanly, otherwise FAILURE
 */
Return sha512sum(
	const int        root_directory_fd,
	const memory     *relative_path,
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

	const char *runtime_relative_path = m_text(relative_path);

	const int file_descriptor = openat(root_directory_fd,runtime_relative_path,O_RDONLY | O_CLOEXEC);

	if(file_descriptor < 0)
	{
		// Flag the read failure
		file->read_error = true;

		// Preserve errno before returning
		file->read_errno = errno;

		provide(status);
	}

	FILE *fileptr = fdopen(file_descriptor,"rb");

	if(fileptr == NULL)
	{
		const int fdopen_errno = errno;

		if(close(file_descriptor) != 0)
		{
			slog(ERROR,"Error closing file descriptor for %s\n",runtime_relative_path);
		}

		file->read_error = true;
		file->read_errno = fdopen_errno;

		provide(status);
	}

	// Move the file pointer checksum_offset bytes from the beginning of the file
	if(fseek(fileptr,file->checksum_offset,SEEK_SET) != 0)
	{
		/*
		 * This looks like an unsupported file type.
		 * Doesn't need to return FAILURE status.
		 */
		file->wrong_file_type = true;
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
		if(sha512_init(&file->mdContext) != CRYPT_OK)
		{
			slog(ERROR,"SHA512 initialization failed\n");
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
	        && testitall_is_huge_interruption_target(runtime_relative_path) == true
	        && file->stat.st_size > 0)
	{
		random_stop_limit = (uint64_t)file->stat.st_size;

		/*
		 * Select interruption target before the first fread() call so
		 * the first chunk can be bounded and the stop point can land
		 * anywhere in [1, file_size].
		 */
		random_stop_byte_value = testitall_random_stop_byte(random_stop_limit);

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

		unsigned char *file_buffer_data_rewritable = m_raw_data(file_buffer);

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
			 * Test-only guard: when random-stop mode is active for hugetestfile,
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

			size_t len = fread(file_buffer_data_rewritable,sizeof(unsigned char),read_limit,fileptr);

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
				if(sha512_update(&file->mdContext,file_buffer_data_rewritable,len) != CRYPT_OK)
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
		slog(ERROR,"Error closing file %s\n",runtime_relative_path);
	}

	if(SUCCESS == status
	        && perform_file_hashing == true
	        && loop_was_interrupted == false)
	{
		file->checksum_offset = 0;

		if(sha512_final(&file->mdContext,file->sha512) != CRYPT_OK)
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
