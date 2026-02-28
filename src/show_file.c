#include "precizer.h"
#include <stdarg.h>

/**
 * @brief Retrieve a const pointer to a flag descriptor by index.
 *
 * Uses the mem helper to obtain a typed readonly view of the Flags array and
 * performs bounds checking. Returns NULL if the descriptor is missing, type
 * verification fails, or the index is out of range.
 */
static const Flags *lookup(
	const memory *flags,
	size_t       index)
{
	const Flags *flags_data = cdata(Flags,flags);

	if(flags_data == NULL || index >= flags->length)
	{
		return(NULL);
	}

	return(&flags_data[index]);
}

/**
 * @brief Prints combinations of change flags for a file.
 *
 * This function evaluates a bitmask of change flags and prints the corresponding descriptions
 * (e.g., "lsize", "asize", "ctime", "mtime") along with their metadata differences. The output uses
 * the provided logger level, mirroring show_metadata behavior.
 *
 * Flag meanings:
 * - lsize: logical file size (`st_size`)
 * - asize: allocated size on disk (POSIX 512-byte blocks via blocks_to_bytes)
 * - ctime: status change timestamp
 * - mtime: content modification timestamp
 *
 */
static void print_changes(
	LOGMODES        level,
	Changed         metadata_of_scanned_and_saved_files,
	const DBrow     *dbrow,
	const CmpctStat *stat)
{
	const char log_level = (char)(level | UNDECOR);

	if(!((rational_logger_mode & VERBOSE) || config->watch_timestamps == true))
	{
		return;
	}

	create(Flags,flags);

	resize(flags,4);

	Flags *flags_data = data(Flags,flags);

	if(flags_data == NULL)
	{
		del(flags);
		return;
	}

	flags_data[0] = (Flags){SIZE_CHANGED,"lsize"};
	flags_data[1] = (Flags){ALLOCATED_SIZE_CHANGED,"asize"};
	flags_data[2] = (Flags){STATUS_CHANGED_TIME,"ctime"};
	flags_data[3] = (Flags){MODIFICATION_TIME_CHANGED,"mtime"};

	unsigned int flags_found = 0;

	/* Check each flag */
	for(size_t i = 0; i < flags->length; i++)
	{
		const Flags *flag = lookup(flags,i);

		if(flag == NULL)
		{
			break;
		}

		if(metadata_of_scanned_and_saved_files & flag->flag_value)
		{
			/*
			 * In the database, in the cell where the CmpctStat structure is stored,
			 * the blkcnt_t st_blocks field of the CmpctStat structure remains unset (not populated
			 * with actual data). This is a migration side effect: in database versions prior to 4,
			 * this field was not persisted.
			 *
			 */
#if 1
			/* This legacy can be removed in 2036 (10-year Long-Term Support) */
			if(flag->flag_value == ALLOCATED_SIZE_CHANGED && dbrow->saved_stat.st_blocks == BLKCNT_UNKNOWN)
#else
			if(flag->flag_value == ALLOCATED_SIZE_CHANGED)
#endif
			{
				continue;
			}

			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				slog(log_level," & ");
			} else {
				slog(log_level,", changed: ");
			}

			slog(log_level,"%s",flag->flag_name);

			show_metadata(level,flag->flag_value,&dbrow->saved_stat,stat);

			flags_found++;
		}
	}

	del(flags);
}

/**
 * @brief Show traversal banners once before the first visible log line of the main pass
 *
 */
static void show_banners(
	bool       *first_iteration,
	const bool stats_only_pass)
{
	if(first_iteration == NULL)
	{
		return;
	}

	bool show_traversal_started = false;
	bool show_update_warning = false;
	bool show_changes_will_be_reflected = false;
	bool show_files_will_be_added = false;

	if(*first_iteration == true)
	{
		*first_iteration = false;

		if(stats_only_pass == false)
		{
			show_traversal_started = true;
	
			if(config->db_contains_data == true)
			{
				if(config->update == true)
				{
					show_update_warning = true;
				}

				show_changes_will_be_reflected = true;

			} else {

				show_files_will_be_added = true;
			}
		}
	}

	if(show_update_warning == true)
	{
		slog(EVERY,"Update mode enabled for DB %s\n",confstr(db_file_name));
	}

	if(show_traversal_started == true)
	{
		slog(EVERY,"File traversal started\n");
	}

	if(show_changes_will_be_reflected == true)
	{
		slog(EVERY,BOLD "Changes reported during this scan against the DB %s:" RESET "\n",confstr(db_file_name));
	}

	if(show_files_will_be_added == true)
	{
		slog(EVERY,BOLD "Items reported during this traversal against the DB %s:" RESET "\n",confstr(db_file_name));
	}
}

/**
 * @brief Log a line with banner/quiet handling and update output flags.
 *
 * Uses the call site metadata from the slog_show macro
 * Banners are emitted only for the main traversal pass
 *
 */
__attribute__((format(printf,9,10)))
void slog_show_impl(
	const char *filename,
	const char *funcname,
	int        line,
	const char level,
	const bool respect_quiet,
	bool       *first_iteration,
	bool       *at_least_one_file_was_shown,
	const bool stats_only_pass,
	const char *fmt,
	...)
{
	if(rational_logger_mode & SILENT)
	{
		return;
	}

	if(respect_quiet == true && config->quiet_ignored == true)
	{
		return;
	}

	show_banners(first_iteration,stats_only_pass);

	if(at_least_one_file_was_shown != NULL)
	{
		*at_least_one_file_was_shown = true;
	}

	char *line_text = NULL;

	va_list args;
	va_start(args,fmt);
	int line_len = vasprintf(&line_text,fmt,args);
	va_end(args);

	if(line_len < 0 || line_text == NULL)
	{
		free(line_text);
		return;
	}

	rational_logger(level,filename,(size_t)line,funcname,"%s",line_text);

	free(line_text);
}

/**
 * @brief Displays the relative path of a file with additional contextual information.
 *
 * This function prints the relative path of a file along with explanations of what actions
 * will be taken regarding the file (e.g., ignore, updated, added, or rehashed). It also handles
 * initial messages for traversal, updates, and warnings, emitted once before the first visible
 * file or directory log, including read errors with errno.
 *
 */
void show_file(
	const DBrow         *dbrow,
	const char          *relative_path,
	const CmpctStat     *stat,
	bool                *first_iteration,
	TraversalSummary    *summary,
	const Changed       metadata_of_scanned_and_saved_files,
	const bool          rehashing_from_the_beginning,
	const bool          ignore,
	const bool          include,
	const bool          locked_checksum_file,
	const bool          lock_checksum_violation,
	const bool          locked_checksum_mismatch,
	const bool          hash_interrupted,
	const sqlite3_int64 checksum_offset,
	const bool          rehash,
	const bool          is_readable,
	const bool          zero_size_file,
	const bool          db_record_inserted,
	const bool          db_record_updated,
	const bool          read_error,
	const int           read_errno)
{
	if(ignore == true)
	{
		if(dbrow->relative_path_was_in_db_before_processing == false)
		{
			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignore & do not add %s\n",relative_path);
		} else {
			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignored & do not update %s\n",relative_path);
		}

	} else if(read_error == true){

		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"error \"%s\" when reading %s\n",strerror(read_errno),relative_path);

	} else if(is_readable == false){

		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"inaccessible file %s\n",relative_path);

	} else if(dbrow->relative_path_was_in_db_before_processing == false){

		/* Add new */

		if(db_record_inserted == true)
		{
			if(include == true)
			{
				slog_show(EVERY|UNDECOR,true,first_iteration,summary,"add included %s\n",relative_path);

			} else if(locked_checksum_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"lock checksum %s\n",relative_path);

			} else if(zero_size_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"add as empty %s\n",relative_path);

			} else {

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"new %s\n",relative_path);
			}
		}

	} else {

		/* Update existing */

		if(locked_checksum_mismatch == true)
		{

			slog_show(EVERY|UNDECOR,false,first_iteration,summary,RED "checksum locked & mismatch, data corrupted" RESET " %s\n",relative_path);

		} else if(lock_checksum_violation == true){

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,RED "checksum locked, data corruption detected" RESET);

			print_changes(EVERY|REMEMBER,metadata_of_scanned_and_saved_files,dbrow,stat);

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary," %s\n",relative_path);

		} else if(db_record_updated == true && include == true){

			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"update included");

			print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);

		} else if(db_record_updated == true && zero_size_file == true){

			slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update as empty");

			print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);

		} else if(rehash == true){

			if(rehashing_from_the_beginning == true)
			{
				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"rehash from the beginning");

				print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);

			} else if(dbrow->saved_offset > 0){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"continue to rehash from %s %s\n",bkbmbgbtbpbeb((const size_t)dbrow->saved_offset,FULL_VIEW),relative_path);

			} else if(locked_checksum_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"locked rehash ok");

				print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);

			} else if(db_record_updated == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update & rehash");

				print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);
			}

		} else if(db_record_updated == true){

			slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update stat");

			print_changes(EVERY,metadata_of_scanned_and_saved_files,dbrow,stat);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",relative_path);
		}
	}

	if(hash_interrupted == true)
	{
		slog_show(EVERY,false,first_iteration,summary,"SHA512 checksum for the file %s has been gracefully interrupted at byte: %s\n",relative_path,bkbmbgbtbpbeb((size_t)checksum_offset,FULL_VIEW));
	}
}
