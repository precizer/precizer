#include "precizer.h"
#include <stdarg.h>

/**
 * @brief Retrieve a const pointer to a flag descriptor by index
 *
 * Uses the mem helper to obtain a typed read-only view of the Flags array and
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
	LOGMODES   level,
	const File *file)
{
	const unsigned int log_level = (unsigned int)(level | UNDECOR);

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

		if(file->db_record_vs_file_metadata_changes & flag->flag_value)
		{
			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				slog(log_level," & ");
			} else {
				slog(log_level,", changed: ");
			}

			slog(log_level,"%s",flag->flag_name);

			show_metadata(level,flag->flag_value,&file->db->saved_stat,&file->stat);

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
	const char         *filename,
	const char         *funcname,
	int                line,
	const unsigned int level,
	const bool         respect_quiet,
	bool               *first_iteration,
	bool               *at_least_one_file_was_shown,
	const bool         stats_only_pass,
	const char         *fmt,
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
 * @brief Show the visible status line for one file path
 *
 * Prints the relative path together with the action or warning that applies to the file.
 * Also triggers the shared traversal banner before the first visible file or directory line
 *
 * @param[in] relative_path Relative path descriptor being reported
 * @param[in,out] first_iteration Banner sentinel for the first visible output line
 * @param[in,out] summary Traversal state used by slog_show()
 * @param[in] file Per-file state that determines which message is printed
 */
void show_file(
	const memory     *relative_path,
	bool             *first_iteration,
	TraversalSummary *summary,
	const File       *file)
{
	const char *runtime_relative_path = getcstring(relative_path);

	if(file->ignore == true)
	{
		if(file->db->relative_path_was_in_db_before_processing == false)
		{
			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignore & do not add %s\n",runtime_relative_path);
		} else {
			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"ignored & do not update %s\n",runtime_relative_path);
		}

	} else if(file->read_error == true){

		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"error \"%s\" when reading %s\n",strerror(file->read_errno),runtime_relative_path);

	} else if(file->is_readable == false){

		slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,"inaccessible file %s\n",runtime_relative_path);

	} else if(file->db->relative_path_was_in_db_before_processing == false){

		/* Add new */

		if(file->new_db_record_inserted == true)
		{
			if(file->include == true)
			{
				slog_show(EVERY|UNDECOR,true,first_iteration,summary,"add included %s\n",runtime_relative_path);

			} else if(file->locked_checksum_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"lock checksum %s\n",runtime_relative_path);

			} else if(file->zero_size_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"add as empty %s\n",runtime_relative_path);

			} else {

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"new %s\n",runtime_relative_path);
			}
		}

	} else {

		/* Update existing */

		if(file->locked_checksum_mismatch == true)
		{

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,RED "checksum locked & mismatch, data corrupted" RESET " %s\n",runtime_relative_path);

		} else if(file->lock_checksum_violation == true){

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary,RED "checksum locked, data corruption detected" RESET);

			print_changes(EVERY|REMEMBER,file);

			slog_show(EVERY|UNDECOR|REMEMBER,false,first_iteration,summary," %s\n",runtime_relative_path);

		} else if(file->db_record_updated == true && file->include == true){

			slog_show(EVERY|UNDECOR,true,first_iteration,summary,"update included");

			print_changes(EVERY,file);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);

		} else if(file->db_record_updated == true && file->zero_size_file == true){

			slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update as empty");

			print_changes(EVERY,file);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);

		} else if(file->rehash == true){

			if(file->rehashing_from_the_beginning == true)
			{
				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"rehash from the beginning");

				print_changes(EVERY,file);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);

			} else if(file->db->saved_offset > 0){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"continue to rehash from %s %s\n",bkbmbgbtbpbeb((const size_t)file->db->saved_offset,FULL_VIEW),runtime_relative_path);

			} else if(file->locked_checksum_file == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"locked rehash ok");

				print_changes(EVERY,file);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);

			} else if(file->db_record_updated == true){

				slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update & rehash");

				print_changes(EVERY,file);

				slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);
			}

		} else if(file->db_record_updated == true){

			slog_show(EVERY|UNDECOR,false,first_iteration,summary,"update stat");

			print_changes(EVERY,file);

			slog_show(EVERY|UNDECOR,false,first_iteration,summary," %s\n",runtime_relative_path);
		}
	}

	if(file->hash_interrupted == true)
	{
		slog_show(EVERY,false,first_iteration,summary,"SHA512 checksum for the file %s has been gracefully interrupted at byte: %s\n",runtime_relative_path,bkbmbgbtbpbeb((size_t)file->checksum_offset,FULL_VIEW));
	}
}
