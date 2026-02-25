#include "precizer.h"

/**
 * @brief Compare two FTS entries by filename
 * @param first Pointer to first FTSENT structure
 * @param second Pointer to second FTSENT structure
 * @return Integer less than, equal to, or greater than zero if first is found,
 *         respectively, to be less than, to match, or be greater than second
 */
static int compare_by_name(
	const FTSENT **first,
	const FTSENT **second)
{
	return strcmp((*first)->fts_name,(*second)->fts_name);
}

static Return match_include_ignore(
	const char *relative_path,
	bool       *include,
	bool       *ignore,
	bool       *include_showed_once,
	bool       *ignore_showed_once)
{
	*include = false;
	*ignore = false;

	Include match_include_response = match_include_pattern(relative_path,include_showed_once);

	if(DO_NOT_INCLUDE == match_include_response)
	{
		Ignore match_ignore_response = match_ignore_pattern(relative_path,ignore_showed_once);

		if(IGNORE == match_ignore_response)
		{
			*ignore = true;

		} else if(FAIL_REGEXP_IGNORE == match_ignore_response){

			slog(ERROR,"Fail ignore REGEXP for a string: %s\n",relative_path);
			provide(FAILURE);
		}

	} else if(FAIL_REGEXP_INCLUDE == match_include_response){

		slog(ERROR,"Fail include REGEXP for a string: %s\n",relative_path);
		provide(FAILURE);

	} else if(INCLUDE == match_include_response){

		*include = true;
	}

	provide(SUCCESS);
}

/**
 * @brief Traverse configured paths and process files for one pass.
 *
 * Supports two modes controlled by summary->stats_only_pass:
 * - true: collect counters and allocated size only;
 * - false: hash files, update DB rows, and collect timing/hash metrics.
 *
 * @param summary Traversal state that is reset and populated by this call.
 * @return SUCCESS, WARNING, or FAILURE.
 */
Return file_list(TraversalSummary *summary)
{
	/// The status that will be passed to provide() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		provide(status);
	}

	if(config->progress == false && summary->stats_only_pass == true)
	{
		// Don't do anything
		provide(status);
	}

	// Flags that reflect the presence of any changes
	// since the last research

	// Print traversal/update banners only once
	bool first_iteration = true;

	// Prevent duplicate --ignore info messages
	bool ignore_showed_once = false;

	// Prevent duplicate --include info messages
	bool include_showed_once = false;

	// Prevent duplicate lock-checksum info messages
	bool lock_checksum_showed_once = false;

	// Signals integrity issues for locked files
	bool lock_checksum_violation_detected = false;

	FTS *file_systems = NULL;
	FTSENT *p = NULL;

	int fts_options = FTS_PHYSICAL;

	if(config->start_device_only == true)
	{
		fts_options |= FTS_XDEV;
	}

	// Reset traversal counters and timing for this pass.
	summary->count_dirs = 0;

	// Number of regular files seen in this pass.
	summary->count_files = 0;

	// Number of symlinks seen in this pass.
	summary->count_symlnks = 0;

	// Sum of allocated bytes for encountered files.
	summary->total_allocated_bytes = 0;

	// Sum of bytes hashed by SHA512 during this pass.
	summary->total_hashed_bytes = 0;

	// Track whether any output was produced
	summary->at_least_one_file_was_shown = false;

	// Sum of per-file hashing elapsed time in nanoseconds.
	summary->total_hashing_elapsed_ns = 0LL;

	if((file_systems = fts_open(config->paths,fts_options,compare_by_name)) == NULL)
	{
		slog(ERROR,"fts_open() error\n");
		provide(FAILURE);
	}

	/*
	 * Determine the absolute path prefix.
	 * We are only interested in relative paths in the database.
	 * To obtain a relative path, trim the prefix from the absolute path.
	 */
	char *runtime_root = NULL;
#if 0 // Old multiPATH solution
	/**
	 * Index of the path prefix
	 * All full runtime paths are stored in the table "paths".
	 * A real path can be retrieved due to its index ID
	 */
	sqlite3_int64 runtime_root_index = -1;
#endif

	// Limit recursion to the depth determined in config->maxdepth
	if(config->maxdepth > -1)
	{
		slog(EVERY,"Recursion depth limited to: %d\n",config->maxdepth);
	}

	if(summary->stats_only_pass == true && config->progress == true)
	{
		slog(EVERY,"File system traversal initiated to calculate file count and storage usage\n");
	}

	bool continue_the_loop = true;

	// Allocate space for a memory structure
	create(unsigned char,file_buffer);

	if(summary->stats_only_pass == false)
	{
		status = resize(file_buffer,file_buffer_memory());

		if(SUCCESS != status)
		{
			provide(status);
		}
	}

#ifdef TESTITALL_TEST_HOOKS
	if(summary->stats_only_pass == false)
	{
		signal_wait_at_point(1U);
	}
#endif

	while((p = fts_read(file_systems)) != NULL && continue_the_loop == true)
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			break;
		}

		/* Get absolute path prefix from FTSENT structure and current runtime path */
		if(p->fts_level == FTS_ROOTLEVEL)
		{
			size_t new_size = (size_t)(p->fts_pathlen + 1) * sizeof(char);

			// All below run once per new path prefix
			char *tmp = (char *)realloc(runtime_root,new_size);

			if(NULL == tmp)
			{
				report("Memory allocation failed, requested size: %zu bytes",new_size);
				status = FAILURE;
				break;
			} else {
				runtime_root = tmp;
			}

			// Remember temporary string in long-lasting variable
			memcpy(runtime_root,p->fts_path,(size_t)p->fts_pathlen);

			if(p->fts_pathlen > 0)
			{
				runtime_root[p->fts_pathlen] = '\0';
			}

			// Remove unnecessary trailing slash at the end of the directory path
			remove_trailing_slash(runtime_root);

#if 0 // Old multiPATH solution
			// If several paths were passed as arguments,
			// then the counting of the path prefix index
			// will start from zero
			if(SUCCESS != (status = db_get_runtime_root_index(config,
				runtime_root,
				&runtime_root_index)))
			{
				continue_the_loop = false;
				break;
			}
#endif
		}

		if(config->maxdepth > -1 && p->fts_level > config->maxdepth + 1)
		{
			if(p->fts_info == FTS_D)
			{
				(void)fts_set(file_systems,p,FTS_SKIP);
			}

			continue;
		}

		switch(p->fts_info)
		{
			case FTS_D:
			{
				const char *relative_path = extract_relative_path(p->fts_path,runtime_root);

				// Captures files explicitly skipped or forced by regexp filters
				// Ignored with --ignore= or admitted with --include=
				bool ignore = false;

				// Included with --include=
				bool include = false;

				status = match_include_ignore(relative_path,
					&include,
					&ignore,
					&include_showed_once,
					&ignore_showed_once);

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				if(summary->stats_only_pass == false)
				{
					directory_show(relative_path,
						&first_iteration,
						summary,
						ignore,
						include);
				}

				if(ignore == true)
				{
					// Skip ignored directories entirely.
					if(config->include_specified == false)
					{
						(void)fts_set(file_systems,p,FTS_SKIP);
					}
					break;
				}

				if(summary->stats_only_pass == false)
				{
					// Check access and skip subtrees that are not readable.
					status = verify_directory_access(file_systems,
						p,
						runtime_root,
						&first_iteration,
						summary);
				}

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				summary->count_dirs++;
				break;
			}
			case FTS_F:
			{
				CmpctStat stat = {0};

				(void)stat_copy(p->fts_statp,&stat);

				summary->total_allocated_bytes += blocks_to_bytes(stat.st_blocks);
				summary->count_files++;

				if(summary->stats_only_pass == true)
				{
					continue;
				}

				/* Write all columns from DB row to the structure DBrow
				   and clean the structure to prevent reuse */
				DBrow _dbrow = {0};
				DBrow *dbrow = &_dbrow;

				const char *relative_path = extract_relative_path(p->fts_path,runtime_root);

				/* Get all file's metadata from the database */
#if 0 // Old multiPATH solution
				run(db_read_file_data_from(dbrow,&runtime_root_index,relative_path));
#else
				run(db_read_file_data_from(dbrow,relative_path));
#endif

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				const bool path_known = dbrow->relative_path_already_in_db == true;

				const bool has_saved_offset = dbrow->saved_offset > 0;

				// Validate whether logical size, allocated blocks, and ctime/mtime
				// changed since the previous scan.
				// Default value is:
				Changed metadata_of_scanned_and_saved_files = NOT_EQUAL;

				// Tracks if the current relative path already has a DB entry
				if(path_known == true)
				{
					// Validate whether logical size, allocated blocks, and ctime/mtime
					// changed since the previous scan.
					metadata_of_scanned_and_saved_files = compare_file_metadata_equivalence(&(dbrow->saved_stat),&stat);
				}

				const bool metadata_identical = metadata_of_scanned_and_saved_files == IDENTICAL;

				const bool metadata_changed = metadata_identical == false;

				// Flag that marks files matched by the checksum lock pattern
				bool locked_checksum_file = false;

				LockChecksum lock_checksum_response = match_checksum_lock_pattern(relative_path,&lock_checksum_showed_once);

				if(FAIL_REGEXP_LOCK_CHECKSUM == lock_checksum_response)
				{
					slog(ERROR,"Fail lock-checksum REGEXP for a string: %s\n",relative_path);
					status = FAILURE;
					continue_the_loop = false;
					break;
				} else if(LOCK_CHECKSUM == lock_checksum_response){
					locked_checksum_file = true;
				}

				// Indicates that the checksum-locked file has already been fully hashed and recorded
				bool lock_checksum_ready = locked_checksum_file == true
				        && path_known == true
				        && has_saved_offset == false;

				// Captures files explicitly skipped or forced by regexp filters
				// Ignored with --ignore= or admitted with --include=
				bool ignore = false;

				// Included with --include=
				bool include = false;

				status = match_include_ignore(relative_path,
					&include,
					&ignore,
					&include_showed_once,
					&ignore_showed_once);

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				// Ensure checksum-locked files are tracked even if matched by ignore pattern
				if(ignore == true && locked_checksum_file == true && path_known == false)
				{
					ignore = false;
				}

				// Determine read access for non-ignored paths
				FileAccessStatus access_status = FILE_ACCESS_DENIED;
				bool is_readable = false;

				/* Check file access */
				if(ignore == false)
				{
					access_status = file_check_access(p->fts_path,(size_t)p->fts_pathlen,R_OK);

					if(access_status == FILE_ACCESS_ERROR)
					{
						status = FAILURE;
						continue_the_loop = false;
						break;
					}

					is_readable = (access_status == FILE_ACCESS_ALLOWED);
				}

				// Used to skip files whose metadata and checksum are already up to date
				bool unchanged_and_complete = path_known == true
				        && metadata_identical == true
				        && has_saved_offset == false;

				if(unchanged_and_complete == true && !(config->rehash_locked == true && lock_checksum_ready == true))
				{
					// Relative path already in DB and doesn't require any change
					break;
				}

				// Derived flags to qualify the type of metadata change
				bool size_changed = (metadata_of_scanned_and_saved_files & SIZE_CHANGED) != 0;

				bool timestamps_changed = (metadata_of_scanned_and_saved_files & (STATUS_CHANGED_TIME | MODIFICATION_TIME_CHANGED)) != 0;

				bool timestamps_only_changed = path_known == true
				        && metadata_changed == true
				        && config->watch_timestamps == false
				        && size_changed == false
				        && has_saved_offset == false;

				// Decision whether to rehash the file contents using
				// the SHA512 algorithm. Defaults to rehash.
				bool rehash = true;

				if(timestamps_only_changed == true)
				{
					// ctime/mtime changed only: update DB without rehash
					rehash = false;
				}

				if(lock_checksum_ready == true && config->rehash_locked == true)
				{
					rehash = true;
				}

				sqlite3_int64 offset = 0;           // Offset bytes
				SHA512_Context mdContext = {0};

				/* For a file which had been changed before creation
				   of its checksum has been already finished */
				bool rehashing_from_the_beginning = false;

				// Can we resume hashing from a previous partial state?
				bool can_resume_partial_hash = has_saved_offset == true
				        && metadata_changed == false;

				// Indicates that a previous partial hash is now invalid and must restart
				bool partial_hash_invalidated = has_saved_offset == true
				        && metadata_changed == true;

				if(can_resume_partial_hash == true)
				{
					// Continue hashing
					offset = dbrow->saved_offset;
					memcpy(&mdContext,&(dbrow->saved_mdContext),sizeof(SHA512_Context));

				} else if(partial_hash_invalidated == true){
					/* The SHA512 hashing of the file had not been
					   finished previously and the file has been changed */
					rehashing_from_the_beginning = true;
				}

				// Marks zero-length files to avoid unnecessary hashing
				bool zero_size_file = false;

				/**
				 * Indicates files that cannot be read/seeks (e.g. sysfs)
				 *
				 * On some special file systems (such as /sys, which has
				 * the SYSFS_MAGIC constant == 0x62656572), standard
				 * file operations like fopen, fseek, and lseek
				 * cannot be used for reading and seeking.
				 * While information about the file itself will be
				 * recorded in the primary database, due to the
				 * nature of such files, their hash sum is never
				 * read and is stored as NULL
				 */
				bool wrong_file_type = false;

				// Read error reported by sha512sum for this path
				bool read_error = false;
				// errno snapshot from the read error (valid when read_error is true).
				int read_errno = 0;

				if(stat.st_size == 0)
				{
					zero_size_file = true;
					rehash = false;
				}

				// Locked checksum files must not diverge once sealed
				bool lock_checksum_violation = lock_checksum_ready == true
				        && (size_changed == true
				        || (config->watch_timestamps == true
				        && config->rehash_locked == false
				        && timestamps_changed == true));

				// Timestamps drift on a locked file may be ignored depending on config
				bool locked_timestamp_drift_only = lock_checksum_ready == true
				        && config->watch_timestamps == false
				        && config->rehash_locked == false
				        && timestamps_changed == true
				        && size_changed == false;

				if(locked_timestamp_drift_only == true)
				{
					break;
				}

				// Buffer for current file SHA512 digest
				unsigned char sha512[SHA512_DIGEST_LENGTH] = {0};

				bool hash_failed = false;
				bool hash_interrupted = false;
				bool locked_checksum_mismatch = false; // Detects corruption when rehashing locked files

				bool should_process = is_readable == true
				        && ignore == false
				        && lock_checksum_violation == false;

				if(should_process == true)
				{
					if(rehash == true)
					{
						if(SUCCESS == status)
						{
							status = sha512sum(p->fts_path,
								(size_t)p->fts_pathlen,
								file_buffer,
								sha512,
								&offset,
								summary,
								&mdContext,
#ifdef TESTITALL_TEST_HOOKS
								stat.st_size,
#endif
								&read_error,
								&read_errno,
								&wrong_file_type);
						}

						if(TRIUMPH & status)
						{
							/* If the sha512sum has been interrupted smoothly when Ctrl+C */
							if(offset > 0 && global_interrupt_flag == true)
							{
								hash_interrupted = true;
							}

						} else {
							continue_the_loop = false;
							hash_failed = true;
						}

					} else {
						memcpy(&sha512,&(dbrow->sha512),sizeof(sha512));
					}

					if(hash_failed == false
					        && read_error == false
					        && config->rehash_locked == true
					        && lock_checksum_ready == true
					        && rehash == true
					        && (TRIUMPH & status)
					        && wrong_file_type == false
					        && zero_size_file == false
					        && offset == 0)
					{
						if(memcmp(sha512,dbrow->sha512,SHA512_DIGEST_LENGTH) != 0)
						{
							locked_checksum_mismatch = true;
						}
					}
				}

				bool db_inserted = false;
				bool db_updated = false;
				bool show_log = false;

				if(is_readable != true
				        || ignore == true
				        || lock_checksum_violation == true
				        || hash_failed == true
				        || read_error == true
				        || locked_checksum_mismatch == true)
				{
					show_log = true;

					/* When a checksum-locked file changed;
					   blocks rehash/DB update and flags corruption */
					if((lock_checksum_violation == true || locked_checksum_mismatch == true) && read_error == false)
					{
						lock_checksum_violation_detected = true;
					}

				} else if(path_known == true){
					/* Update in DB */

					bool allow_locked_update = lock_checksum_violation == false
					        && (locked_checksum_file == false
					        || config->rehash_locked == true
					        || has_saved_offset == true);

					bool should_update_db = path_known == true
					        && allow_locked_update == true
					        && (offset > dbrow->saved_offset
					        || (has_saved_offset == true && offset == 0)
					        || metadata_changed == true);

					if(should_update_db == true)
					{
						/* Update record in DB */
						if(TRIUMPH & status)
						{
							status = db_update_the_record_by_id(&(dbrow->ID),
								&offset,
								sha512,
								&stat,
								&mdContext,
								&zero_size_file,
								&wrong_file_type);

							if((TRIUMPH & status) == 0)
							{
								continue_the_loop = false;
								break;
							}
							db_updated = true;
						}
					}

					show_log = true;
				} else {

					/* Insert into DB */
					if(TRIUMPH & status)
					{
#if 0 // Old multiPATH solution
						status = db_insert_the_record(&runtime_root_index,
							relative_path,
							&offset,
							sha512,
							&stat,
							&mdContext,
							&zero_size_file,
							&wrong_file_type);
#else
						status = db_insert_the_record(relative_path,
							&offset,
							sha512,
							&stat,
							&mdContext,
							&zero_size_file,
							&wrong_file_type);
#endif

						if((TRIUMPH & status) == 0)
						{
							continue_the_loop = false;
							break;
						}
						db_inserted = true;
					}

					show_log = true;
				}

				if(show_log == true)
				{
					// Print out of a file name and its changes
					show_file(dbrow,
						relative_path,
						&stat,
						&first_iteration,
						summary,
						metadata_of_scanned_and_saved_files,
						rehashing_from_the_beginning,
						ignore,
						include,
						locked_checksum_file,
						lock_checksum_violation,
						locked_checksum_mismatch,
						hash_interrupted,
						offset,
						rehash,
						is_readable,
						zero_size_file,
						db_inserted,
						db_updated,
						read_error,
						read_errno);
				}

				break;
			}
			case FTS_SL:
				summary->count_symlnks++;
				break;
			case FTS_DNR:
			case FTS_ERR:
			case FTS_NS:
			{
				if(summary->stats_only_pass == true)
				{
					break;
				}

				const char *relative_path = extract_relative_path(p->fts_path,runtime_root);

				if(p->fts_info == FTS_DNR)
				{
					slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"inaccessible directory %s\n",relative_path);

				} else if(p->fts_info == FTS_NS){

					slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"cannot stat \"%s\" when reading %s\n",strerror(p->fts_errno),relative_path);

				} else {

					slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"fts error \"%s\" when reading %s\n",strerror(p->fts_errno),relative_path);
				}

				break;
			}
			default:
				break;
		}
	}

	del(file_buffer);

	free(runtime_root);

	fts_close(file_systems);

	// Print completion banner only when traversal emitted visible path-level lines.
	// Print preflight totals only for the stats-only pass from main().
	if(SUCCESS == status)
	{
		if(summary->at_least_one_file_was_shown == true)
		{

			slog(EVERY,"File traversal complete\n");
		}

		if(summary->stats_only_pass == true)
		{
			show_statistics(summary);
		}
	}

	if(lock_checksum_violation_detected == true)
	{
		slog(EVERY,BOLD "Warning! Data corruption detected for checksum-locked file!" RESET "\n");

		if(SUCCESS == status)
		{
			status = WARNING;
		}
	}

	provide(status);
}
