#include "precizer.h"

/**
 * @brief Display statistics for filesystem components
 *
 */
static void display_statistics(
	size_t       *count_dirs,
	size_t       *count_files,
	size_t       *count_symlnks,
	size_t const *total_size_in_bytes,
	const bool   *count_size_of_all_files,
	const bool   *at_least_one_file_was_shown)
{
	size_t total_items = *count_dirs + *count_files + *count_symlnks;

	bool show_total = false;
	bool show_complete = false;

	if(*count_size_of_all_files == true)
	{
		show_total = true;

	} else if(*at_least_one_file_was_shown == true){

		show_complete = true;
		show_total = true;
	}

	if(show_complete == true)
	{
		slog(EVERY,"File traversal complete\n");
	}

	if(show_total == true)
	{
		slog(EVERY,"Total size: %s, total items: %zu, dirs: %zu, files: %zu, symlnks: %zu\n",
			bkbmbgbtbpbeb(*total_size_in_bytes),
			total_items,
			*count_dirs,
			*count_files,
			*count_symlnks);
	}
}

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

/**
 *
 * Traverses a directory recursively and returns
 * a struct for each file it encounters
 *
 */
Return file_list(const bool count_size_of_all_files)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	if(config->progress == false && count_size_of_all_files == true)
	{
		// Don't do anything
		return(status);
	}

	// Flags that reflect the presence of any changes
	// since the last research
	bool first_iteration = true;
	bool ignore_showed_once = false;
	bool include_showed_once = false;
	bool lock_checksum_showed_once = false;
	bool at_least_one_file_was_shown = false;
	bool lock_checksum_detected = false;

	FTS *file_systems = NULL;
	FTSENT *p = NULL;

	int fts_options = FTS_PHYSICAL;

	if(config->start_device_only == true)
	{
		fts_options |= FTS_XDEV;
	}

	size_t count_files = 0,count_dirs = 0,count_symlnks = 0,total_size_in_bytes = 0;

	if((file_systems = fts_open(config->paths,fts_options,compare_by_name)) == NULL)
	{
		slog(ERROR,"fts_open() error\n");
		fts_close(file_systems);
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

	if(count_size_of_all_files == true && config->progress == true)
	{
		slog(EVERY,"File system traversal initiated to calculate file count and storage usage\n");
	}

	bool continue_the_loop = true;

	// Allocate space for a memory structure
	create(unsigned char,file_buffer);

	if(count_size_of_all_files == false)
	{
		status = resize(file_buffer,file_buffer_memory());

		if(SUCCESS != status)
		{
			provide(status);
		}
	}

	while((p = fts_read(file_systems)) != NULL && continue_the_loop == true)
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true)
		{
			break;
		}

		if(count_size_of_all_files == false)
		{
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
				strcpy(runtime_root,p->fts_path);

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
		}

		switch(p->fts_info)
		{
			case FTS_D:
				count_dirs++;
				break;
			case FTS_F:
			{
				// Limit recursion to the depth determined in config->maxdepth
				if(config->maxdepth > -1 && p->fts_level > config->maxdepth + 1)
				{
					break;
				}

				CmpctStat stat = {0};

				(void)stat_copy(p->fts_statp,&stat);

				total_size_in_bytes += (size_t)stat.st_size;
				count_files++;

				if(runtime_root == NULL)
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

				// Validate if size, creation and modification time of a
				// file has not changed since last scanning.
				// Default value is:
				Changed metadata_of_scanned_and_saved_files = NOT_EQUAL;

				if(dbrow->relative_path_already_in_db == true)
				{
					// Validate if size, creation and modification time of a
					// file has not changed since last scanning.
					metadata_of_scanned_and_saved_files = compare_file_metadata_equivalence(&(dbrow->saved_stat),&stat);
				}

				bool unchanged_and_complete = dbrow->relative_path_already_in_db == true
				        && metadata_of_scanned_and_saved_files == IDENTICAL
				        && dbrow->saved_offset == 0;

				if(unchanged_and_complete == true)
				{
					// Relative path already in DB and doesn't require any change
					break;
				}

				bool timestamps_only_changed = dbrow->relative_path_already_in_db == true
				        && metadata_of_scanned_and_saved_files != IDENTICAL
				        && config->watch_timestamps == false
				        && !(metadata_of_scanned_and_saved_files & SIZE_CHANGED)
				        && dbrow->saved_offset == 0;

				// Decision whether to rehash the file contents using
				// the SHA512 algorithm. Defaults to Yes, rehash"
				bool rehash = true;

				if(timestamps_only_changed == true)
				{
					// ctime/mtime changed only: update DB without rehash
					rehash = false;
				}

				bool locked_checksum_file = false;

				LockChecksum lock_checksum_response = match_checksum_lock_pattern(relative_path,&lock_checksum_showed_once);

				if(FAIL_REGEXP_LOCK_CHECKSUM == lock_checksum_response)
				{
					slog(ERROR,"Fail lock-checksum REGEXP for a string: %s",relative_path);
					status = FAILURE;
					continue_the_loop = false;
					break;
				} else if(LOCK_CHECKSUM == lock_checksum_response){
					locked_checksum_file = true;
				}

				sqlite3_int64 offset = 0;           // Offset bytes
				SHA512_Context mdContext = {0};

				/* For a file which had been changed before creation
				   of its checksum has been already finished */
				bool rehashing_from_the_beginning = false;

				bool can_resume_partial_hash = dbrow->saved_offset > 0
				        && metadata_of_scanned_and_saved_files == IDENTICAL;

				bool partial_hash_invalidated = dbrow->saved_offset > 0
				        && metadata_of_scanned_and_saved_files != IDENTICAL;

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

				// The file is available for reading
				bool is_readable = false;

				/* Check file access */
				status = file_check_access(p->fts_path,
					(size_t)p->fts_pathlen,
					&is_readable);

				if(SUCCESS != status)
				{
					continue_the_loop = false;
					break;
				}

				bool zero_size_file = false;

				/**
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

				if(p->fts_statp->st_size == 0)
				{
					zero_size_file = true;
					rehash = false;
				}

				// Ignored with --ignore= or admitted with --include=
				bool ignore = false;

				// Included with --include=
				bool include = false;

				/* PCRE2 regexp to include the file */
				Include match_include_response = match_include_pattern(relative_path,&include_showed_once);

				if(DO_NOT_INCLUDE == match_include_response)
				{
					/* PCRE2 regexp to ignore the file */

					Ignore match_ignore_response = match_ignore_pattern(relative_path,&ignore_showed_once);

					if(IGNORE == match_ignore_response)
					{
						ignore = true;

					} else if(FAIL_REGEXP_IGNORE == match_ignore_response){
						slog(ERROR,"Fail ignore REGEXP for a string: %s",relative_path);
						status = FAILURE;
						continue_the_loop = false;
						break;
					}

				} else if(FAIL_REGEXP_INCLUDE == match_include_response){
					slog(ERROR,"Fail include REGEXP for a string: %s",relative_path);
					status = FAILURE;
					continue_the_loop = false;
					break;
				} else if(INCLUDE == match_include_response){
					include = true;
				}

				// Ensure checksum-locked files are tracked even if matched by ignore pattern
				if(ignore == true && locked_checksum_file == true && dbrow->relative_path_already_in_db == false)
				{
					ignore = false;
				}

				// Path is checksum-locked, already stored, and its hash was fully calculated previously
				bool lock_checksum_violation = locked_checksum_file == true
				        && dbrow->relative_path_already_in_db == true
				        && dbrow->saved_offset == 0
				        && metadata_of_scanned_and_saved_files != IDENTICAL;

				// Print out of a file name and its changes
				show_relative_path(relative_path,
					&metadata_of_scanned_and_saved_files,
					dbrow,
					&stat,
					&first_iteration,
					&rehashing_from_the_beginning,
					&ignore,
					&include,
					&locked_checksum_file,
					&lock_checksum_violation,
					&at_least_one_file_was_shown,
					&rehash,
					&count_size_of_all_files,
					&is_readable,
					&zero_size_file);

				if(is_readable != true)
				{
					break;
				}

				if(ignore == true)
				{
					break;
				}

				/* When a checksum-locked file changed;
				   blocks rehash/DB update and flags corruption */
				if(lock_checksum_violation == true)
				{
					lock_checksum_detected = true;
					break;
				}

				// Buffer for current file SHA512 digest
				unsigned char sha512[SHA512_DIGEST_LENGTH] = {0};

				if(rehash == true)
				{
					run(sha512sum(p->fts_path,
						(size_t)p->fts_pathlen,
						file_buffer,
						sha512,
						&offset,
						&mdContext,
						&wrong_file_type));

					if(TRIUMPH & status)
					{
						/* If the sha512sum has been interrupted smoothly when Ctrl+C */
						if(offset > 0 && global_interrupt_flag == true)
						{
							slog(EVERY,"SHA512 checksum for the file %s has been"
								" gracefully interrupted at byte: %s\n",
								relative_path,
								bkbmbgbtbpbeb((size_t)offset));
						}

					} else {
						continue_the_loop = false;
						break;
					}

				} else {
					memcpy(&sha512,&(dbrow->sha512),sizeof(sha512));
				}

				if(dbrow->relative_path_already_in_db == true)
				{
					/* Update in DB */

					bool should_update_db = dbrow->relative_path_already_in_db == true
					        && locked_checksum_file == false
					        && (offset > dbrow->saved_offset
					        || (dbrow->saved_offset > 0 && offset == 0)
					        || metadata_of_scanned_and_saved_files != IDENTICAL);

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

							if(SUCCESS != status)
							{
								continue_the_loop = false;
								break;
							}
						}
					}

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

						if(SUCCESS != status)
						{
							continue_the_loop = false;
							break;
						}
					}
				}

				/**
				 * Interrupt the loop smoothly
				 * Interrupt when Ctrl+C
				 */
				if(global_interrupt_flag == true)
				{
					break;
				}
			}
			break;
			case FTS_SL:
				count_symlnks++;
				break;
			default:
				break;
		}
	}

	del(file_buffer);

	free(runtime_root);

	fts_close(file_systems);

	// Display statistics for filesystem components
	if(SUCCESS == status)
	{
		display_statistics(&count_dirs,
			&count_files,
			&count_symlnks,
			&total_size_in_bytes,
			&count_size_of_all_files,
			&at_least_one_file_was_shown);
	}

	if(lock_checksum_detected == true)
	{
		slog(ERROR,BOLD "Caution! Data corruption detected for checksum-locked file!" RESET "\n");
		status = WARNING;
	}

	provide(status);
}
