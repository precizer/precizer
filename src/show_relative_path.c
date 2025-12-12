#include "precizer.h"

/**
 * @brief Prints combinations of change flags for a file.
 *
 * This function evaluates a bitmask of change flags and prints the corresponding descriptions
 * (e.g., "size", "ctime", "mtime") along with their metadata differences. It also indicates
 * whether the file will be rehashed.
 *
 */
static void print_flag_combinations(
	int             mega,
	const DBrow     *dbrow,
	const CmpctStat *stat,
	const bool      *rehash)
{
	const char *flags[] = {
		"size","ctime","mtime"
	};
	const int flag_values[] = {
		SIZE_CHANGED,STATUS_CHANGED_TIME,MODIFICATION_TIME_CHANGED
	};
	const int flag_count = 3;
	unsigned int flags_found = 0;
	bool first_word = true;

	/* Check each flag */
	for(int i = 0; i < flag_count; i++)
	{
		if(mega & flag_values[i])
		{
			if(first_word == true)
			{
				printf(" ");
				first_word = false;
			}

			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				printf(" & ");
			}
			printf("%s",flags[i]);
			show_metadata(i,&dbrow->saved_stat,stat);
			flags_found++;
		}
	}

	if(*rehash == true)
	{
		if(dbrow->saved_offset > 0)
		{
			printf(" continue to rehash from %s",bkbmbgbtbpbeb((const size_t)dbrow->saved_offset));
		} else {
			printf(" rehash");
		}
	} else {
		printf(" no rehash");
	}
}

/**
 * @brief Prints details about updated or added files.
 *
 * This function outputs information about files that have been updated or newly added to the database.
 * It includes metadata changes and rehashing status.
 *
 */
static void print_updated_or_added(
	const int       *metadata_of_scanned_and_saved_files,
	const DBrow     *dbrow,
	const CmpctStat *stat,
	const bool      *rehash)
{
	if(dbrow->relative_path_already_in_db == true)
	{
		printf(" updated");
		print_flag_combinations(*metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
	} else {
		printf(" add");
	}
}

/**
 * @brief Prints details about changed files.
 *
 * This function outputs information about files that have changed, including metadata differences
 * and rehashing status.
 *
 */
static void print_changed(
	const int       *metadata_of_scanned_and_saved_files,
	const DBrow     *dbrow,
	const CmpctStat *stat,
	const bool      *rehash)
{
	if(dbrow->relative_path_already_in_db == true)
	{
		printf(" changed");
		print_flag_combinations(*metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
	}
}

/**
 * @brief Displays the relative path of a file with additional contextual information.
 *
 * This function prints the relative path of a file along with explanations of what actions
 * will be taken regarding the file (e.g., ignored, updated, added, or rehashed). It also handles
 * initial messages for traversal, updates, and warnings.
 *
 */
void show_relative_path(
	const char      *relative_path,
	const int       *metadata_of_scanned_and_saved_files,
	const DBrow     *dbrow,
	const CmpctStat *stat,
	bool            *first_iteration,
	bool            *show_changes,
	const bool      *rehashing_from_the_beginning,
	const bool      *ignored,
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
			*show_changes = false;
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
		slog(EVERY,BOLD "These files have been added or changed and those changes will be reflected against the DB %s:" RESET "\n",config->db_file_name);
	}

	if(show_files_will_be_added == true)
	{
		slog(EVERY,BOLD "These files will be added against the %s database:" RESET "\n",config->db_file_name);
	}

	// Print if NOT silent
	if(!(rational_logger_mode & SILENT))
	{
		*at_least_one_file_was_shown = true;

		if(*is_readable == false)
		{
			printf("%s %s\n",relative_path,"inaccessible");

		} else if(*ignored == true){
			printf("%s %s\n",relative_path,"ignored & not added");

		} else if(*zero_size_file == true){
			printf("%s %s\n",relative_path,"zero size");

		} else if(*ignored == false){
			printf("%s",relative_path);

			if(*rehashing_from_the_beginning)
			{
				printf(" the SHA512 hashing of the file had not been finished previously, since then the file has been changed and will be rehashed from the beginning\n");
			} else {
				if(*show_changes == true)
				{
					if(config->watch_timestamps == true)
					{
						if(*metadata_of_scanned_and_saved_files != IDENTICAL)
						{
							print_changed(metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
						} else {
							print_updated_or_added(metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
						}
					} else {

						if(*metadata_of_scanned_and_saved_files & SIZE_CHANGED)
						{
							print_changed(metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
						} else {
							print_updated_or_added(metadata_of_scanned_and_saved_files,dbrow,stat,rehash);
						}
					}
				}
				printf("\n");
			}
		} else {
			printf("\n");
		}
	}
}
