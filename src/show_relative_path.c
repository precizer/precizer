#include "precizer.h"

/**
 *
 * Print out the relative path of the file
 * with additional explanations of what
 * exactly will happen to this file
 *
 */
void show_relative_path
(
	const char *relative_path,
	const int *metadata_of_scanned_and_saved_files,
	const DBrow *dbrow,
	bool *first_iteration,
	bool *show_changes,
	bool *rehashig_from_the_beginning,
	bool *ignored
){

	if(*first_iteration == true)
	{
		*first_iteration = false;

		if(config->db_already_exists == true)
		{
			if(config->update == true)
			{
				slog(false,"The \033[1m--update\033[m option has been used, so the information about files will be updated against the database %s\n",config->db_file_name);
			}

			slog(false,"\033[1mThese files have been added or changed and those changes will be reflected against the DB %s:\n\033[m",config->db_file_name);

		} else {
			*show_changes = false;
			slog(false,"\033[1mThese files will be added against the DB %s:\n\033[m",config->db_file_name);
		}
	}

	if(config->silent == false)
	{
		if(*ignored == true)
		{
			printf("\033[1mignored\033[0m ");
		}

		printf("%s",relative_path);

		if(*ignored == false)
		{
			if(*rehashig_from_the_beginning)
			{
				printf(" the SHA512 hashing of the file had not been finished previously, the file has been changed and will be rehashed from the begining\n");
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
								break;
							case 2:
								printf("ctime");
								break;
							case 3:
								printf("size & ctime");
								break;
							case 4:
								printf("mtime");
								break;
							case 5:
								printf("size & mtime");
								break;
							case 6:
								printf("ctime & mtime");
								break;
							case 7:
								printf("size & ctime & mtime");
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
