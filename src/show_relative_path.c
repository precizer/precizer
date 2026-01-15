#include "precizer.h"

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
 * (e.g., "size", "ctime", "mtime") along with their metadata differences. The output uses
 * the provided logger level, mirroring show_metadata behavior.
 *
 */
static void print_changes(
	LOGMODES        level,
	Changed         change_flags_mask,
	const DBrow     *dbrow,
	const CmpctStat *stat)
{
	const char log_level = (char)(level | UNDECOR);

	if(!((rational_logger_mode & VERBOSE) || config->watch_timestamps == true))
	{
		return;
	}

	create(Flags,flags);

	resize(flags,3);

	Flags *flags_data = data(Flags,flags);

	if(flags_data == NULL)
	{
		del(flags);
		return;
	}

	flags_data[0] = (Flags){SIZE_CHANGED,"size"};
	flags_data[1] = (Flags){STATUS_CHANGED_TIME,"ctime"};
	flags_data[2] = (Flags){MODIFICATION_TIME_CHANGED,"mtime"};

	unsigned int flags_found = 0;

	/* Check each flag */
	for(size_t i = 0; i < flags->length; i++)
	{
		const Flags *flag = lookup(flags,i);

		if(flag == NULL)
		{
			break;
		}

		if(change_flags_mask & flag->flag_value)
		{
			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				slog(log_level," & ");
			} else {
				slog(log_level," changed ");
			}

			slog(log_level,"%s",flag->flag_name);

			show_metadata(level,flag->flag_value,&dbrow->saved_stat,stat);

			flags_found++;
		}
	}

	del(flags);
}

/**
 * @brief Displays the relative path of a file with additional contextual information.
 *
 * This function prints the relative path of a file along with explanations of what actions
 * will be taken regarding the file (e.g., ignore, updated, added, or rehashed). It also handles
 * initial messages for traversal, updates, and warnings.
 *
 */
void show_relative_path(
	const char      *relative_path,
	const Changed   *metadata_of_scanned_and_saved_files,
	const DBrow     *dbrow,
	const CmpctStat *stat,
	bool            *first_iteration,
	const bool      *rehashing_from_the_beginning,
	const bool      *ignore,
	const bool      *include,
	const bool      *locked_checksum_file,
	const bool      *lock_checksum_violation,
	bool            *at_least_one_file_was_shown,
	const bool      *rehash,
	const bool      *count_size_of_all_files,
	const bool      *is_readable,
	const bool      *zero_size_file)
{
	bool show_traversal_started = false;
	bool show_update_warning = false;
	bool show_changes_will_be_reflected = false;
	bool show_files_will_be_added = false;

	if(*first_iteration == true)
	{
		*first_iteration = false;

		if(*count_size_of_all_files == false)
		{
			show_traversal_started = true;
		}

		if(config->db_contains_data == true)
		{
			if(config->update == true)
			{
				show_update_warning = true;
				config->the_update_warning_has_already_been_shown = true;
			}

			show_changes_will_be_reflected = true;

		} else {

			show_files_will_be_added = true;
		}
	}

	if(show_update_warning == true)
	{
		slog(EVERY,"The " BOLD "--update" RESET " option has been used, so the information about files will be updated against the database %s\n",config->db_file_name);
	}

	if(show_traversal_started == true)
	{
		slog(EVERY,"File traversal started\n");
	}

	if(show_changes_will_be_reflected == true)
	{
		slog(EVERY,BOLD "These files were added or changed on disk and will be reflected against the DB %s:" RESET "\n",config->db_file_name);
	}

	if(show_files_will_be_added == true)
	{
		slog(EVERY,BOLD "These files will be added against the %s database:" RESET "\n",config->db_file_name);
	}

	// Print if NOT silent
	if(!(rational_logger_mode & SILENT))
	{
		*at_least_one_file_was_shown = true;
	}

	/* Prefixes */

	if(*is_readable == false)
	{
		slog(EVERY|UNDECOR,"%s %s\n","inaccessible",relative_path);

	/* Add or update */

	} else if(dbrow->relative_path_already_in_db == false){

		/* Add new */

		if(*ignore == true)
		{
			slog(EVERY|UNDECOR,"%s %s\n","ignore & do not add",relative_path);

		} else if(*include == true){

			slog(EVERY|UNDECOR,"%s %s\n","add included",relative_path);

		} else if(*locked_checksum_file == true){

			slog(EVERY|UNDECOR,"%s %s\n","lock checksum",relative_path);

		} else if(*zero_size_file == true){

			slog(EVERY|UNDECOR,"%s %s\n","add as empty",relative_path);

		} else {

			slog(EVERY|UNDECOR,"%s %s\n","add",relative_path);
		}

	} else {

		/* Update existing */

		if(*ignore == true)
		{
			slog(EVERY|UNDECOR,"ignored & do not update %s\n",relative_path);

		} else if(*include == true){

			slog(EVERY|UNDECOR,"update included");

			print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);

			slog(EVERY|UNDECOR," %s\n",relative_path);

		} else if(*lock_checksum_violation == true){

			slog(EVERY|UNDECOR|REMEMBER,RED "checksum locked, data corruption detected" RESET);

			print_changes(EVERY|REMEMBER,*metadata_of_scanned_and_saved_files,dbrow,stat);

			slog(EVERY|UNDECOR|REMEMBER," %s\n",relative_path);

		} else if(*zero_size_file == true){

			slog(EVERY|UNDECOR,"update as empty");

			print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);

			slog(EVERY|UNDECOR," %s\n",relative_path);

		} else if(*rehash == true){

			if(*rehashing_from_the_beginning == true)
			{
				slog(EVERY|UNDECOR,"rehash from the beginning");

				print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);

				slog(EVERY|UNDECOR," %s\n",relative_path);

			} else if(dbrow->saved_offset > 0){
				slog(EVERY|UNDECOR,"continue to rehash from %s %s\n",bkbmbgbtbpbeb((const size_t)dbrow->saved_offset),relative_path);

			} else if(*locked_checksum_file == true){

				slog(EVERY|UNDECOR,"rehash locked");

				if(*metadata_of_scanned_and_saved_files != IDENTICAL)
				{
					print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);
				}

				slog(EVERY|UNDECOR," %s\n",relative_path);

			} else {

				slog(EVERY|UNDECOR,"update & rehash");

				print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);

				slog(EVERY|UNDECOR," %s\n",relative_path);
			}

		} else {
			slog(EVERY|UNDECOR,"update stat");

			print_changes(EVERY,*metadata_of_scanned_and_saved_files,dbrow,stat);

			slog(EVERY|UNDECOR," %s\n",relative_path);
		}
	}
}
