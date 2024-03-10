#include "precizer.h"
#include <fts.h>

/**
 *
 * Traverses a directory recursively and returns
 * a struct for each file it encounters
 *
*/
Return file_list
(
	bool count_size_of_all_files
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything
	if(config->compare == true)
	{
		return(status);
	}

	if(config->progress == false && count_size_of_all_files == true){
		// Don't do anything
		return(status);
	}

	// Flags that reflect the presence of any changes
	// since the last research
	bool first_iteration = true;
	bool show_changes = true;

	FTS *file_systems = NULL;
	FTSENT *p = NULL;
	FTSENT *child = NULL;

	int fts_options = FTS_PHYSICAL | FTS_XDEV;

	size_t count_files = 0, count_dirs = 0, count_symlnks = 0;

	if ((file_systems = fts_open(config->paths, fts_options, NULL)) == NULL) {
		slog(false,"fts_open error\n");
		status = FAILURE;
		fts_close(file_systems);
		return(status);
	}

	/* Initialize file_systems with as many argv[] parts as possible. */
	child = fts_children(file_systems, 0);
	if (child == NULL) {
		 /* no files to traverse */
		fts_close(file_systems);
		return(status);
	}

	/*
	 * Determine absolute path prefix
	 * We are only interested in relative paths in DB
	 * To get a relative path we need to trim the prefix from the absolute path
	 */
	char *runtime_path_prefix = NULL;
	FTSENT *current_file_system = child;

	while((p = fts_read(file_systems)) != NULL)
	{
		/* Interrupt the loop smoothly */
		/* Interrupt when Ctrl+C */
		if(global_interrupt_flag == true){
			break;
		}

		if (count_size_of_all_files == false)
		{
			/* Get absolute path prefix from FTSENT structure and current runtime path */
			if (p == current_file_system){
				// All below run once per new path prefix
				char *tmp = (char *)realloc(runtime_path_prefix,(current_file_system->fts_pathlen + 1) * sizeof(char));
				if(NULL == tmp)
				{
					slog(false,"Realloc error\n");
					status = FAILURE;
					break;
				} else {
					runtime_path_prefix = tmp;
				}

				// Remember temporary string in long-lasting variable
				strcpy(runtime_path_prefix,current_file_system->fts_path);

				// Remove unnecessary trailing slash at the end of the directory path
				remove_trailing_slash(runtime_path_prefix);

				// The next
				current_file_system = current_file_system->fts_link;
			}
		}

		switch (p->fts_info) {
		case FTS_D:
			count_dirs++;
			break;
		case FTS_F:
			{
				if (count_size_of_all_files == true){
					config->total_size_in_bytes += (size_t)p->fts_statp->st_size;
					count_files++;
				} else if(runtime_path_prefix != NULL)
				{
					const char *relative_path = p->fts_path + strlen(runtime_path_prefix) + 1 + correction(p->fts_path + strlen(runtime_path_prefix) + 1);
					struct stat *stat = p->fts_statp;
					count_files++;

					/* Write all columns from DB row to the structure DBrow */
					DBrow _dbrow;
					DBrow *dbrow = &_dbrow;
					
					/* Get all file's metadata from the database */
					if(SUCCESS != (status = db_read_file_data_from(dbrow,relative_path)))
					{
						break;
					}

					// Check up if size, creation and modification time of a
					// file has not changed since last scanning.
					int metadata_of_scanned_and_saved_files = NOT_EQUAL;

					if(dbrow->relative_path_already_in_db == true)
					{
						// Check up if size, creation and modification time of a
						// file has not changed since last scanning.
						metadata_of_scanned_and_saved_files = compare_file_metadata_equivalence(&(dbrow->saved_stat),p->fts_statp);

						// The file metadata in DB and on the file system are identical
						if(metadata_of_scanned_and_saved_files == IDENTICAL)
						{
							// The file saved against the database has been read
							// from the file system in its entirety
							if(dbrow->saved_offset == 0){
								// Relative path already in DB and doesn't need any change
								break;
							}
						}
					}

					sqlite3_int64 offset = 0; // Offset bytes
					SHA512_Context mdContext;

					// For a file which had been changed before creation of its checksum has been already finished.
					bool rehashig_from_the_beginning = false;

					// Reflect changes in global
					config->something_has_been_changed = true;

					if(dbrow->saved_offset > 0)
					{
						if (metadata_of_scanned_and_saved_files == IDENTICAL)
						{
							// Contunue hashing
							offset = dbrow->saved_offset;
							memcpy(&mdContext,&(dbrow->saved_mdContext),sizeof(SHA512_Context));
						} else {
							// The SHA512 hashing of the file had not been finished previously and the file has been changed
							rehashig_from_the_beginning = true;
						}
					}

					unsigned char sha512[SHA512_DIGEST_LENGTH];
					memset(sha512,0,sizeof(sha512)); // Clean sha512 to prevent reuse;

					// Print out of a file name and its changes
					show_relative_path(relative_path,&metadata_of_scanned_and_saved_files,dbrow,&first_iteration,&show_changes,&rehashig_from_the_beginning);

					if(SUCCESS != (status = sha512sum(p->fts_path,&p->fts_pathlen,sha512,&offset,&mdContext)))
					{
						break;
					}

					bool update_db = false;

					if (dbrow->relative_path_already_in_db == true)
					{
						if(offset > dbrow->saved_offset)
						{
							// Update DB record
							update_db = true;

						} else if(dbrow->saved_offset > 0 && offset == 0)
						{
							// Update DB record
							update_db = true;

						} else if(metadata_of_scanned_and_saved_files != IDENTICAL)
						{
							// Update DB record
							update_db = true;
						}
					}

					/* In any other case NO need to update DB record just insert the record */
					if(update_db == true)
					{
						/* Update record in DB */
						if(SUCCESS != (status = db_update_the_record(&(dbrow->ID),&offset,sha512,stat,&mdContext)))
						{
							break;
						}
					} else {
						/* Insert to DB */
						if(SUCCESS != (status = db_insert_the_record(relative_path,&offset,sha512,stat,&mdContext)))
						{
							break;
						}
					}

					/**
					 * Interrupt the loop smoothly
					 * Interrupt when Ctrl+C
					 * Don't write a result because sha512sum() function
					 * has been interrupted and the sha512 contains wrong data
					*/
					if(global_interrupt_flag == true){
						break;
					}
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

	free(runtime_path_prefix);

	fts_close(file_systems);

	size_t total_items = count_dirs + count_files + count_symlnks;

	if(config->progress == true)
	{
		slog(false,"total size: %s, total items: %zu, dirs: %zu, files: %zu, symlnks: %zu\n",bkbmbgbtbpbeb(config->total_size_in_bytes), total_items, count_dirs,count_files,count_symlnks);
	}

	return(status);
}
