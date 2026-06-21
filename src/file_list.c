#include "precizer.h"

/**
 * @brief Compare two FTS entries by filename
 * @param first Pointer to first FTSENT structure
 * @param second Pointer to second FTSENT structure
 * @return Integer less than, equal to, or greater than zero if first is found,
 *         respectively, to be less than, to match, or be greater than second
 */
static int compare_by_name(
#ifdef __CYGWIN__
	const FTSENT * const *first,
	const FTSENT * const *second)
#else
	const FTSENT **first,
	const FTSENT **second)
#endif
{
	return strcmp((*first)->fts_name,(*second)->fts_name);
}

/**
 * @brief Traverse configured roots and process files for one pass
 *
 * The function is a no-op in compare mode. During normal scanning it walks each
 * root in `config->roots` with a separate FTS stream. Paths written to the
 * database are kept relative to the current root, so the same traversal logic
 * works for one root or for several positional directory arguments
 *
 * `summary->stats_only_pass` controls how much work is done. When it is `true`,
 * traversal only counts directories, files, symlinks, and allocated size, and
 * that pass is skipped unless progress output needs those counters. When it is
 * `false`, regular files can be hashed, compared with existing database rows,
 * inserted, updated, or reported according to ignore/include and checksum
 * locking settings
 *
 * For example, after `precizer src tests`, `config->roots` contains two roots.
 * This function traverses `src` first, closes that FTS stream, then traverses
 * `tests`
 *
 * @param summary Traversal state that is reset and populated by this call
 * @return `SUCCESS` when traversal completes, `WARNING` when a non-fatal
 *         checksum-lock violation is reported, or `FAILURE` for hard traversal,
 *         memory, hashing, or database errors
 */
Return file_list(TraversalSummary *summary)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		provide(status);
	}

#ifdef TESTITALL_TEST_HOOKS
	{
		const char *skip = getenv("TESTITALL_TEST_ENV_SKIP_FILE_LIST");

		if(skip != NULL && strcmp(skip,"1") == 0)
		{
			provide(status);
		}
	}
#endif

	if(config->progress == false && summary->stats_only_pass == true)
	{
		// Don't do anything
		provide(status);
	}

	// Flags that reflect the presence of any changes
	// since the last research

	// Print traversal/update banners only once
	bool first_iteration = true;

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

	/*
	 * Determine the absolute path prefix.
	 * We are only interested in relative paths in the database.
	 * To obtain a relative path, trim the prefix from the absolute path.
	 */
	m_create(char,root_path,MEMORY_STRING);
#if 0 // Disabled multi-root path index implementation
	/**
	 * Index of the path prefix
	 * All full runtime paths are stored in the table "paths".
	 * A real path can be retrieved due to its index ID
	 */
	sqlite3_int64 runtime_root_path_index = -1;
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
	m_create(unsigned char,file_buffer);
	m_create(char,relative_path,MEMORY_STRING);

	if(summary->stats_only_pass == false)
	{
		status = m_resize(file_buffer,file_buffer_memory());

		if(SUCCESS != status)
		{
			provide(status);
		}
#ifdef TESTITALL_TEST_HOOKS
		signal_wait_at_point(1U);
#endif
	}

	// Open one FTS stream per traversal root
	m_string_array_foreach(conf(roots),root)
	{
		char *root_path_text = m_data(char,root);

		if(root_path_text == NULL)
		{
			slog(ERROR,"Unable to access root path descriptor\n");
			status = FAILURE;
			break;
		}

		char *root_argv[] = {
			root_path_text,
			NULL
		};

		if((file_systems = fts_open(root_argv,fts_options,compare_by_name)) == NULL)
		{
			slog(ERROR,"fts_open() error\n");
			status = FAILURE;
			break;
		}

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
				run(m_copy_fixed_string(root_path,(size_t)p->fts_pathlen + 1U,p->fts_path));

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				// Remove unnecessary trailing slash at the end of the directory path
				run(remove_trailing_slash(root_path));

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

	#if 0 // Disabled multi-root path index implementation
				// If several paths were passed as arguments,
				// then the counting of the path prefix index
				// will start from zero
				if(SUCCESS != (status = db_get_runtime_root_path_index(config,
					root_path,
					&runtime_root_path_index)))
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
					run(extract_relative_path(relative_path,p->fts_path,(size_t)p->fts_pathlen,root_path));

					if((TRIUMPH & status) == 0)
					{
						continue_the_loop = false;
						break;
					}

					// Captures files explicitly skipped or forced by regexp filters
					// Ignored with --ignore= or admitted with --include=
					bool ignore = false;

					// Included with --include=
					bool include = false;

					status = match_include_ignore(relative_path,&include,&ignore);

					if((TRIUMPH & status) == 0)
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
						/*
						 * Use FTS_SKIP for an ignored directory only when no --include
						 * or --lock-checksum patterns were provided
						 * When checksum locking is active, descendants of an ignored
						 * directory may still need integrity checks or rehashing
						 */
						if(config->include_specified == false
						        && config->lock_checksum == NULL)
						{
							(void)fts_set(file_systems,p,FTS_SKIP);
						}
						break;
					}

					if(summary->stats_only_pass == false)
					{
						// Check access and skip subtrees that are not readable.
						status = directory_access_verify(file_systems,p,root_path,&first_iteration,summary);
					}

					if((TRIUMPH & status) == 0)
					{
						continue_the_loop = false;
						break;
					}

					summary->count_dirs++;
					break;
				}
				case FTS_F:
				{
					summary->total_allocated_bytes += blocks_to_bytes(p->fts_statp->st_blocks);
					summary->count_files++;

					if(summary->stats_only_pass == true)
					{
						continue;
					}

					// Keep per-file processing state on the stack and attach
					// the DB row that will be filled for this path
					File _file = {0};
					File *file = &_file;
					DBrow dbrow = {0};
					file->db = &dbrow;

					run(extract_relative_path(relative_path,p->fts_path,(size_t)p->fts_pathlen,root_path));

					if((TRIUMPH & status) == 0)
					{
						continue_the_loop = false;
						break;
					}

					/* Get all file's metadata from the database */
#if 0 // Disabled multi-root path index implementation
					run(db_read_file_data_from(file,&runtime_root_path_index,relative_path));
#else
					run(db_read_file_data_from(file,relative_path));
#endif

					if(SUCCESS != status)
					{
						continue_the_loop = false;
						break;
					}

					// Tracks whether the current relative path existed in DB before current file processing
					const bool path_known = file->db->relative_path_was_in_db_before_processing == true;

					/*
					 * YES means the current relative path is checksum-locked and the file must be marked as protected.
					 * NO means the path is not checksum-locked, so the file remains a regular tracked file
					 */
					if(ask(path_check_locked_checksum(relative_path)))
					{
						// Mark this file as checksum-locked
						file->locked_checksum_file = true;
					}

					if(SUCCESS != status)
					{
						continue_the_loop = false;
						break;
					}

					// Store the final --include/--ignore decision in the per-file state
					status = match_include_ignore(relative_path,&file->include,&file->ignore);

					if(SUCCESS != status)
					{
						continue_the_loop = false;
						break;
					}

					// Ensure checksum-locked files stay visible even if matched by ignore
					if(file->ignore == true && file->locked_checksum_file == true)
					{
						file->ignore = false;
					}

					// Determine read access for non-ignored paths
					if(file->ignore == false)
					{
						FileAccessStatus access_status = file_check_access_absolute(p->fts_path,(size_t)p->fts_pathlen,R_OK);
						bool locked_unavailable_violation = false;

						if(path_known == true
						        && file->locked_checksum_file == true
						        && access_status != FILE_ACCESS_ALLOWED)
						{
							status = show_locked_checksum_unavailable_violation(relative_path,
								access_status,
								&first_iteration,
								summary);

							if(SUCCESS != status)
							{
								continue_the_loop = false;
								break;
							}

							file->is_readable = false;
							locked_unavailable_violation = true;

						} else {
							file->is_readable = (access_status == FILE_ACCESS_ALLOWED);
						}

						if(locked_unavailable_violation == true)
						{
							lock_checksum_violation_detected = true;
							break;
						}
					}

					// Copy the current metadata once for comparisons, hashing, logging,
					// and database writes
					run(stat_copy(p->fts_statp,&file->stat));

					// Validate whether logical size, allocated blocks, and ctime/mtime
					// changed since the previous scan.
					// Default value is:
					file->db_record_vs_file_metadata_changes = NOT_EQUAL;

					if(path_known == true)
					{
						// Validate whether logical size, allocated blocks, and ctime/mtime
						// changed since the previous scan.
						file->db_record_vs_file_metadata_changes = file_compare_metadata_equivalence(&file->db->saved_stat,&file->stat);
					}

					bool file_metadata_identical = file->db_record_vs_file_metadata_changes == IDENTICAL;
					const bool has_saved_offset = file->db->saved_offset > 0;
					// Indicates that the checksum-locked file has already been fully hashed and recorded
					bool lock_checksum_ready = file->locked_checksum_file == true
					        && path_known == true
					        && has_saved_offset == false;

					// Used to skip files whose metadata and checksum are already up to date
					bool unchanged_and_complete = path_known == true
					        && file_metadata_identical == true
					        && has_saved_offset == false;

					if(unchanged_and_complete == true && !(config->rehash_locked == true && lock_checksum_ready == true))
					{
						// Relative path already in DB and doesn't require any change
						break;
					}

					// Derived flags to qualify the type of metadata change
					bool size_changed = (file->db_record_vs_file_metadata_changes & SIZE_CHANGED) != 0;

					bool timestamps_changed = (file->db_record_vs_file_metadata_changes & (STATUS_CHANGED_TIME | MODIFICATION_TIME_CHANGED)) != 0;

					bool timestamps_only_changed = path_known == true
					        && file_metadata_identical == false
					        && config->watch_timestamps == false
					        && size_changed == false
					        && has_saved_offset == false;

					// Decision whether to rehash the file contents using
					// the SHA512 algorithm. Defaults to rehash.
					file->rehash = true;

					if(timestamps_only_changed == true)
					{
						// ctime/mtime changed only: update DB without rehash
						file->rehash = false;
					}

					if(lock_checksum_ready == true && config->rehash_locked == true)
					{
						file->rehash = true;
					}

					/* For a file which had been changed before creation
					   of its checksum has been already finished */

					// Can we resume hashing from a previous partial state?
					bool can_resume_partial_hash = has_saved_offset == true
					        && file_metadata_identical == true;

					// Indicates that a previous partial hash is now invalid and must restart
					bool partial_hash_invalidated = has_saved_offset == true
					        && file_metadata_identical == false;

					if(can_resume_partial_hash == true)
					{
						// Continue hashing from the saved offset and SHA512 context
						// Restore byte offset from where the previous pass stopped
						file->checksum_offset = file->db->saved_offset;
						// Restore SHA512 state to continue from that offset
						memcpy(&file->mdContext,&file->db->saved_mdContext,sizeof(SHA512_Context));

					} else if(partial_hash_invalidated == true){
						/* The SHA512 hashing of the file had not been
						   finished previously and the file has been changed */
						// Signal that hashing must restart from byte zero
						file->rehashing_from_the_beginning = true;
					}

					// Marks zero-length files to avoid unnecessary hashing
					if(file->stat.st_size == 0)
					{
						file->zero_size_file = true;
						file->rehash = false;
					}

					// Locked checksum files must not diverge once sealed
					file->lock_checksum_violation = lock_checksum_ready == true
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

					// Hard hashing failure that should stop traversal for this pass
					bool hash_failed = false;
					// Controls whether the final outcome for this file should be shown
					bool show_log = false;

					bool should_process = file->is_readable == true
					        && file->ignore == false
					        && file->lock_checksum_violation == false;

					if(should_process == true)
					{
						if(file->rehash == true)
						{
							if(SUCCESS == status)
							{
								status = sha512sum(p->fts_path,
									(size_t)p->fts_pathlen,
									file_buffer,
									summary,
									file);
							}

							if(TRIUMPH & status)
							{
								/* If the sha512sum has been interrupted smoothly when Ctrl+C */
								if(file->checksum_offset > 0 && global_interrupt_flag == true)
								{
									file->hash_interrupted = true;
								}

							} else {
								continue_the_loop = false;
								hash_failed = true;
							}

						} else {
							// No rehash: reuse the digest stored in the DB
							memcpy(file->sha512,file->db->sha512,sizeof(file->sha512));
						}

						if(hash_failed == false
						        && file->read_error == false
						        && config->rehash_locked == true
						        && lock_checksum_ready == true
						        && file->rehash == true
						        && (TRIUMPH & status)
						        && file->wrong_file_type == false
						        && file->zero_size_file == false
						        && file->checksum_offset == 0)
						{
							if(memcmp(file->sha512,file->db->sha512,SHA512_DIGEST_LENGTH) != 0)
							{
								// Detects corruption when rehashing locked files
								file->locked_checksum_mismatch = true;
							}
						}
					}

					if(file->is_readable != true
					        || file->ignore == true
					        || file->lock_checksum_violation == true
					        || hash_failed == true
					        || file->read_error == true
					        || file->locked_checksum_mismatch == true)
					{
						show_log = true;

						/* When a checksum-locked file changed;
						   blocks rehash/DB update and flags corruption */
						if((file->lock_checksum_violation == true || file->locked_checksum_mismatch == true) && file->read_error == false)
						{
							lock_checksum_violation_detected = true;
							/*
							 * A checksum-locked content or metadata mismatch means protected data may be corrupted.
							 * Replay remembered warnings at exit even without --progress so this critical result remains visible after normal traversal output
							 */
							config->show_remembered_messages_at_exit = true;
						}

					} else if(path_known == true){
						/* Update in DB */
						show_log = true;

						bool allow_locked_update = file->locked_checksum_file == false
						        || config->rehash_locked == true
						        || has_saved_offset == true;

						bool should_update_db = allow_locked_update == true
						        && (file->checksum_offset > file->db->saved_offset
						        || (has_saved_offset == true && file->checksum_offset == 0)
						        || file_metadata_identical == false);

						if(should_update_db == true)
						{
							/* Update record in DB */
							if(TRIUMPH & status)
							{
								status = db_update_the_record_by_id(file);

								if((TRIUMPH & status) == 0)
								{
									continue_the_loop = false;
									break;
								}
								// Record that the DB row was updated
								file->db_record_updated = true;
							}
						}
					} else {
						show_log = true;

						/* Insert into DB */
						if(TRIUMPH & status)
						{
#if 0 // Disabled multi-root path index implementation
							status = db_insert_the_record(&runtime_root_path_index,
								relative_path,
								file);
#else
							status = db_insert_the_record(relative_path,file);
#endif

							if((TRIUMPH & status) == 0)
							{
								continue_the_loop = false;
								break;
							}
							// Record that a new DB row was inserted
							file->new_db_record_inserted = true;
						}
					}

					if(show_log == true)
					{
						// Print out of a file name and its changes
						show_file(relative_path,
							&first_iteration,
							summary,
							file);
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

					run(extract_relative_path(relative_path,p->fts_path,(size_t)p->fts_pathlen,root_path));

					if((TRIUMPH & status) == 0)
					{
						continue_the_loop = false;
						break;
					}

					const char *runtime_relative_path = m_text(relative_path);

					if(p->fts_info == FTS_DNR)
					{
						slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"inaccessible directory %s\n",runtime_relative_path);

					} else if(p->fts_info == FTS_NS){

						slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"cannot stat \"%s\" when reading %s\n",strerror(p->fts_errno),runtime_relative_path);

					} else {

						slog_show(EVERY|UNDECOR|REMEMBER,false,&first_iteration,summary,"fts error \"%s\" when reading %s\n",strerror(p->fts_errno),runtime_relative_path);
					}

					break;
				}
				default:
					break;
			}
		}

		if(file_systems != NULL)
		{
			fts_close(file_systems);
			file_systems = NULL;
		}

		if(global_interrupt_flag == true || (TRIUMPH & status) == 0)
		{
			break;
		}
	}

	m_del(relative_path);
	m_del(file_buffer);

	m_del(root_path);

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
