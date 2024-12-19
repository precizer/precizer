 #include "precizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_metadata(
	int flag,
	const struct stat *was,
	const struct stat *now
){
	if(rational_logger_mode & (VERBOSE|TESTING))
	{
		switch(flag)
		{
			case 0:
					printf(" was:%s",bkbmbgbtbpbeb((size_t)was->st_size));
					printf(", now:%s",bkbmbgbtbpbeb((size_t)now->st_size));
				break;
			case 1:
					printf(" was:%s.%ld",seconds_to_ISOdate(was->st_ctim.tv_sec),was->st_ctim.tv_nsec);
					printf(", now:%s.%ld",seconds_to_ISOdate(now->st_ctim.tv_sec),now->st_ctim.tv_nsec);
				break;
			case 2:
					printf(" was:%s.%ld",seconds_to_ISOdate(was->st_mtim.tv_sec),was->st_mtim.tv_nsec);
					printf(", now:%s.%ld",seconds_to_ISOdate(now->st_mtim.tv_sec),now->st_mtim.tv_nsec);
				break;
			default:
					return;
				break;
		}
	}
}

/**
 * @brief Prints combinations of flags based on bit values
 * @param[in] mega Integer containing combined flag values
 */
static void print_flag_combinations(
	int mega,
	const DBrow *dbrow,
	const struct stat *fts_statp
){
	const char *flags[] = {"size", "ctime", "mtime"};
	const int flag_values[] = {SIZE_CHANGED, CREATION_TIME_CHANGED, MODIFICATION_TIME_CHANGED};
	const int flag_count = 3;
	int flags_found = 0;

	if(mega == 0)
	{
		printf("No flags set\n");
		return;
	}

	/* Check each flag */
	for(int i = 0; i < flag_count; i++)
	{
		if(mega & flag_values[i])
		{
			/* Add separator if not the first flag */
			if(flags_found > 0)
			{
				printf(" & ");
			}
			printf("%s",flags[i]);
			print_metadata(i,&dbrow->saved_stat,fts_statp);
			flags_found++;
		}
	}
}

/**
 *
 * Print out the relative path of the file
 * with additional explanations of what
 * exactly will happen to this file
 * @arg @c relative_path Relative path by itself
 * @arg @c metadata_of_scanned_and_saved_files Code of changes in file metadata
 *
 */
void show_relative_path(
	const char        *relative_path,
	const int         *metadata_of_scanned_and_saved_files,
	const DBrow       *dbrow,
	const struct stat *fts_statp,
	bool              *first_iteration,
	bool              *show_changes,
	bool              *rehashig_from_the_beginning,
	const bool        *ignored,
	bool              *at_least_one_file_was_shown
){
	if(*first_iteration == true)
	{
		*first_iteration = false;

		if(config->db_contains_data == true)
		{
			if(config->update == true)
			{
				slog(EVERY,"The " BOLD "--update" RESET " option has been used, so the information about files will be updated against the database %s\n",config->db_file_name);
			}

			slog(EVERY,BOLD "These files have been added or changed and those changes will be reflected against the DB %s:" RESET "\n",config->db_file_name);

		} else {
			*show_changes = false;
			slog(EVERY,BOLD "These files will be added against the %s database:" RESET "\n",config->db_file_name);
		}
	}

	// If NOT silent
	if(!(rational_logger_mode & SILENT))
	{
		if(*ignored == true)
		{
			printf(BOLD "ignored " RESET);
		}

		printf("%s",relative_path);

		*at_least_one_file_was_shown = true;

		if(*ignored == false)
		{
			if(*rehashig_from_the_beginning)
			{
				printf(" the SHA512 hashing of the file had not been finished previously, since then the file has been changed and will be rehashed from the beginning\n");
			} else {
				if(*show_changes == true)
				{
					if(*metadata_of_scanned_and_saved_files != IDENTICAL
					        && dbrow->relative_path_already_in_db == true)
					{
						printf(" changed ");
						print_flag_combinations(*metadata_of_scanned_and_saved_files,dbrow,fts_statp);
					} else {
						if(dbrow->relative_path_already_in_db == true)
						{
							printf(" updating");
						} else {
							printf(" adding");
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
