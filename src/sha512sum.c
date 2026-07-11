#include "precizer.h"
#include <errno.h>
#include <fcntl.h>

/*
 * Minimum elapsed monotonic time between periodic hash checkpoints.
 * The value is stored in nanoseconds; 15 seconds bounds worst-case lost hashing progress after an unexpected crash without writing to SQLite too often
 */
static const long long int sha512_checkpoint_interval_ns = 14930016475LL;

/**
 * @brief Check whether periodic database checkpoints are safe for this hash pass
 *
 * Periodic checkpoints are skipped in dry-run mode because no persistent DB
 * state should be changed. They are also skipped for a fully sealed
 * checksum-locked file, so no control path can replace the stored final
 * checksum with a partial hash state
 *
 * @param[in] file Per-file state currently being hashed
 * @return true when a periodic checkpoint may be written, otherwise false
 */
static bool periodic_hash_checkpoint_is_allowed(const File *file)
{
	if(config->dry_run == true)
	{
		return false;
	}

	if(file->checksum_offset <= 0)
	{
		return false;
	}

	/*
	 * A sealed checksum-locked row must keep its trusted final checksum and metadata.
	 * Even if a future control path reaches sha512sum(), periodic checkpoints must
	 * not replace it with temporary offset/mdContext state
	 */
	if(file->locked_checksum_file == true
	        && file->db->relative_path_was_in_db_before_processing == true
	        && file->db->saved_offset == 0)
	{
		return false;
	}

	return true;
}

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
 * @param[in,out] path_known True when the row currently exists in SQLite.
 *                           Periodic checkpoints may flip this to true after
 *                           inserting a partial row for a previously unknown path
 * @return SUCCESS when the file was handled cleanly, otherwise FAILURE
 */
Return sha512sum(
	const int        root_directory_fd,
	const memory     *relative_path,
	memory           *file_buffer,
	TraversalSummary *summary,
	File             *file,
	bool             *path_known)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(file_buffer->length == 0 || path_known == NULL)
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

	/*
	 * Shared random-stop state for SHA512 test hooks.
	 * Test builds use these values to coordinate byte-exact interruption,
	 * byte-exact checkpointing, and the optional crash-after-checkpoint scenario
	 */
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
	bool testitall_checkpoint_at_random_byte = testitall_hash_checkpoint_at_random_byte_enabled();
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

	/*
	 * Test hook A: choose the byte where the test should interfere.
	 * Later hook comments refer to hook A because signal waiting, read limiting,
	 * and byte-forced checkpoints all need the same exact target byte
	 */
#ifdef TESTITALL_TEST_HOOKS
	/*
	 * Activate random interruption for a fresh pass of hugetestfile.
	 * Byte-checkpoint tests may also select a stop point after resume, so
	 * checkpoint-update paths can be exercised without a signal
	 */
	if((file->checksum_offset == 0 || testitall_checkpoint_at_random_byte == true)
	        && testitall_is_huge_interruption_target(runtime_relative_path) == true
	        && file->stat.st_size > 0)
	{
		const uint64_t file_size = (uint64_t)file->stat.st_size;
		uint64_t current_offset = 0U;

		if(file->checksum_offset > 0)
		{
			current_offset = (uint64_t)file->checksum_offset;
		}

		if(current_offset < file_size)
		{
			const uint64_t remaining_file_size = file_size - current_offset;

			if(testitall_checkpoint_at_random_byte == true && remaining_file_size <= 1U)
			{
				random_stop_limit = 0U;

			} else {
				random_stop_limit = file_size;

				/*
				 * Select interruption target before the first fread() call so
				 * the first chunk can be bounded and the stop point can land
				 * anywhere in the remaining file range.
				 */
				random_stop_byte_value = current_offset + testitall_random_stop_byte(remaining_file_size);

				/* Defensive fallback: never allow a zero stop byte. */
				if(random_stop_byte_value == 0U)
				{
					random_stop_byte_value = 1U;
				}

				/*
				 * Keep the first byte-checkpoint stop at least two bytes before EOF.
				 * This leaves room for the resume pass to write another checkpoint
				 * before the final hash state is reached
				 */
				if(testitall_checkpoint_at_random_byte == true
				        && current_offset == 0U
				        && random_stop_limit > 2U
				        && random_stop_byte_value >= random_stop_limit - 1U)
				{
					random_stop_byte_value = random_stop_limit - 2U;

				} else if(random_stop_limit > 1U && random_stop_byte_value >= random_stop_limit){
					/*
					 * Keep the stop point strictly inside file data for multi-byte files.
					 * If random selection lands exactly at EOF, shift it one byte left.
					 * The guard keeps subtraction safe and avoids unsigned underflow
					 */
					random_stop_byte_value = random_stop_limit - 1U;
				}

				if(random_stop_byte_value <= current_offset && current_offset + 1U < random_stop_limit)
				{
					random_stop_byte_value = current_offset + 1U;
				}
			}
		}
	}
#endif

	if(perform_file_hashing == true)
	{
		long long int hashing_start_ns = cur_time_monotonic_ns();

		/*
		 * Monotonic timestamp when the next periodic checkpoint should be considered.
		 * It is advanced after each checkpoint window so the database is not updated on every read block
		 */
		long long int next_checkpoint_ns = hashing_start_ns + sha512_checkpoint_interval_ns;

		unsigned char *file_buffer_data_rewritable = m_raw_data(file_buffer);

		while(true)
		{
			/*
			 * Test hook B: notify the signal-driven interruption test after the byte
			 * selected by hook A has been hashed. The delay hook uses this as the
			 * proof that the controlled stop point was really reached
			 */
#ifdef TESTITALL_TEST_HOOKS
			/*
			 * Trigger point 2 exactly once when selected stop byte is reached.
			 */
			if(random_stop_limit > 0U
			        && testitall_checkpoint_at_random_byte == false
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
			/*
			 * Test hook C: decide whether normal Ctrl+C/SIGINT handling must wait.
			 * This protects the signal-driven random-stop test until hook B has
			 * reached the byte selected by hook A
			 */
#ifdef TESTITALL_TEST_HOOKS
			/*
			 * Test-only guard: when random-stop mode is active for hugetestfile,
			 * do not break on global_interrupt_flag until point 2 has really
			 * happened. Otherwise interruption may fire too early and miss the
			 * controlled "interrupt at random byte" scenario.
			 */
			bool delay_interrupt_for_random_stop = false;

			if(random_stop_limit > 0U
			        && testitall_checkpoint_at_random_byte == false
			        && random_stop_triggered == false)
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

			/*
			 * The normal interrupt condition also honors hook C in test builds.
			 * This keeps production behavior intact while letting the test reach
			 * the controlled stop byte before breaking the read loop
			 */
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

			/*
			 * Cap one fread() call so the loop lands exactly on the byte selected
			 * by hook A. Without this cap, a large read could jump past the target
			 * and make the checkpoint/resume test flaky
			 */
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

				/*
				 * Test hook D: request an immediate checkpoint when the byte selected
				 * by hook A has just been hashed. The checkpoint condition and crash
				 * simulation use this flag to avoid waiting for the time interval
				 */
#ifdef TESTITALL_TEST_HOOKS
				bool testitall_checkpoint_now = false;

				if(testitall_checkpoint_at_random_byte == true
				        && random_stop_limit > 0U
				        && random_stop_triggered == false
				        && random_stop_byte_value > 0U
				        && (uint64_t)(file->checksum_offset) >= random_stop_byte_value)
				{
					testitall_checkpoint_now = true;
					random_stop_triggered = true;
				}
#endif

				/*
				 * Current monotonic timestamp sampled after the SHA512 state and byte offset were advanced.
				 * Saving only after this point keeps offset and mdContext consistent in the database
				 */
				const long long int checkpoint_now_ns = cur_time_monotonic_ns();

				/*
				 * Honor hook D by entering the checkpoint path immediately.
				 * This is what makes byte-precise checkpoint tests practical even
				 * when the normal time-based checkpoint interval has not elapsed
				 */
				if(checkpoint_now_ns >= next_checkpoint_ns
#ifdef TESTITALL_TEST_HOOKS
				        || testitall_checkpoint_now == true
#endif
				)
				{
					/*
					 * Test hook E: remember whether this checkpoint really reached
					 * the database. The crash simulation relies on hook E so it only
					 * exits after there is durable state to resume from
					 */
#ifdef TESTITALL_TEST_HOOKS
					bool checkpoint_saved = false;
#endif

					if(periodic_hash_checkpoint_is_allowed(file) == true)
					{
						status = db_save_file_record(relative_path,file,path_known,false);

						if((TRIUMPH & status) == 0)
						{
							break;
						}

						/*
						 * Set hook E only after db_save_file_record() succeeds.
						 * This connects the real DB write above with the crash
						 * simulation below
						 */
#ifdef TESTITALL_TEST_HOOKS
						checkpoint_saved = true;
#endif
					}

					/*
					 * Simulate a sudden process death only when hook D requested this
					 * byte checkpoint and hook E proves it was saved. The next test run
					 * can then prove that the stored partial SHA512 state survived
					 */
#ifdef TESTITALL_TEST_HOOKS
					if(checkpoint_saved == true
					        && testitall_checkpoint_now == true
					        && testitall_exit_after_hash_checkpoint_enabled() == true)
					{
						testitall_exit_after_hash_checkpoint();
					}
#endif

					next_checkpoint_ns = checkpoint_now_ns + sha512_checkpoint_interval_ns;
				}
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

	provide(status);
}
