#include "precizer.h"

static void print_size(
	const struct stat *was,
	const struct stat *now
){
	if(config->verbose == true)
	{
		printf(" was:%s",bkbmbgbtbpbeb((ui64)was->st_size));
		printf(", now:%s",bkbmbgbtbpbeb((ui64)now->st_size));
	}
}

static void print_ctim(
	const struct stat *was,
	const struct stat *now
){
	if(config->verbose == true)
	{
		printf(" was:%s.%ld",seconds_to_ISOdate(was->st_ctim.tv_sec),was->st_ctim.tv_nsec);
		printf(", now:%s.%ld",seconds_to_ISOdate(now->st_ctim.tv_sec),now->st_ctim.tv_nsec);
	}
}

static void print_mtim(
	const struct stat *was,
	const struct stat *now
){
	if(config->verbose == true)
	{
		printf(" was:%s.%ld",seconds_to_ISOdate(was->st_mtim.tv_sec),was->st_mtim.tv_nsec);
		printf(", now:%s.%ld",seconds_to_ISOdate(now->st_mtim.tv_sec),now->st_mtim.tv_nsec);
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
void show_relative_path
(
	const char *relative_path,
	const int *metadata_of_scanned_and_saved_files,
	const DBrow *dbrow,
	const struct stat *fts_statp,
	bool *first_iteration,
	bool *show_changes,
	bool *rehashig_from_the_beginning,
	const bool *ignored,
	bool *at_least_one_file_was_shown
){
	if(*first_iteration == true)
	{
		*first_iteration = false;

		if(config->db_already_exists == true)
		{
			if(config->update == true)
			{
				slog(false,"The \033[1m--update\033[0m option has been used, so the information about files will be updated against the database %s\n",config->db_file_name);
			}

			slog(false,"\033[1mThese files have been added or changed and those changes will be reflected against the DB %s:\n\033[0m",config->db_file_name);

		} else {
			*show_changes = false;
			slog(false,"\033[1mThese files will be added against the %s database:\n\033[0m",config->db_file_name);
		}
	}

	if(config->silent == false)
	{
		if(*ignored == true)
		{
			printf("\033[1mignored\033[0m ");
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
						switch(*metadata_of_scanned_and_saved_files) {
							case 1:
								printf("size");
								print_size(&dbrow->saved_stat,fts_statp);
								break;
							case 2:
								printf("ctime");
								print_ctim(&dbrow->saved_stat,fts_statp);
								break;
							case 3:
								printf("size");
								print_size(&dbrow->saved_stat,fts_statp);
								printf(" & ctime");
								print_ctim(&dbrow->saved_stat,fts_statp);
								break;
							case 4:
								printf("mtime");
								print_mtim(&dbrow->saved_stat,fts_statp);
								break;
							case 5:
								printf("size");
								print_size(&dbrow->saved_stat,fts_statp);
								printf(" & mtime");
								print_mtim(&dbrow->saved_stat,fts_statp);
								break;
							case 6:
								printf("ctime");
								print_ctim(&dbrow->saved_stat,fts_statp);
								printf(" & mtime");
								print_mtim(&dbrow->saved_stat,fts_statp);
								break;
							case 7:
								printf("size");
								print_size(&dbrow->saved_stat,fts_statp);
								printf(" & ctime");
								print_ctim(&dbrow->saved_stat,fts_statp);
								printf(" & mtime");
								print_mtim(&dbrow->saved_stat,fts_statp);
								break;
							default:
								break;
						}
					} else {
						if (dbrow->relative_path_already_in_db == true)
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
