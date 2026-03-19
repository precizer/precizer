/**
 * @file db_insert_the_record.c
 * @brief Implementation of database record insertion functionality
 * @details Contains functions for inserting new records into the SQLite database
 *          including file metadata, checksums and offset information
 */

#include "precizer.h"

/**
 * @brief Inserts a new record into the database
 *
 * @details This function inserts information about a file including its relative path,
 *          file offset, SHA512 checksum, file metadata (stat), and SHA512 context
 *          into the SQLite database. The function handles both complete file records
 *          and partial records where some fields may be NULL.
 *
 * @param[in] relative_path Path to the file relative to the root directory
 * @param[in] file Per-file state object. Uses checksum_offset, sha512, stat,
 *                 mdContext, zero_size_file, and wrong_file_type when binding
 *                 the row values
 *
 * @return Return status code:
 *         - SUCCESS: Record inserted successfully
 *         - FAILURE: Error occurred during insertion
 *
 * @note In dry run mode, the function returns SUCCESS without modifying the database
 */
Return db_insert_the_record(
	const char *relative_path,
	const File *file)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Skip database operations in dry run mode --dry-run */
	if(config->dry_run == true)
	{
		provide(status);
	}

	int rc = 0;

	sqlite3_stmt *insert_stmt = NULL;

#if 0 // Old multiPATH solution
	const char *insert_sql = "INSERT INTO files (offset,path_prefix_index,relative_path,sha512,stat,mdContext) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
#else
	const char *insert_sql = "INSERT INTO files (offset,relative_path,sha512,stat,mdContext) VALUES (?1, ?2, ?3, ?4, ?5);";
#endif

	/* Prepare SQL statement */
	rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&insert_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to prepare insert statement %s",insert_sql);
		status = FAILURE;
	}

	/* Bind offset value */
	if(SUCCESS == status)
	{
		if(file->checksum_offset == 0)
		{
			rc = sqlite3_bind_null(insert_stmt,1);
		} else {
			rc = sqlite3_bind_int64(insert_stmt,1,file->checksum_offset);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind offset value");
			status = FAILURE;
		}
	}

	/* Bind relative path */
	if(SUCCESS == status)
	{
		rc = sqlite3_bind_text(insert_stmt,2,relative_path,(int)strlen(relative_path),NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind relative path");
			status = FAILURE;
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS == status)
	{
		if(file->checksum_offset == 0 && file->zero_size_file == false && file->wrong_file_type == false)
		{
			rc = sqlite3_bind_blob(insert_stmt,3,file->sha512,SHA512_DIGEST_LENGTH,NULL);
		} else {
			rc = sqlite3_bind_null(insert_stmt,3);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind SHA512 checksum");
			status = FAILURE;
		}
	}

	/* Copy and bind file metadata */
	if(SUCCESS == status)
	{
		rc = sqlite3_bind_blob(insert_stmt,4,&file->stat,sizeof(CmpctStat),NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind file metadata");
			status = FAILURE;
		}
	}

	/* Bind SHA512 context */
	if(SUCCESS == status)
	{
		if(file->checksum_offset == 0)
		{
			rc = sqlite3_bind_null(insert_stmt,5);
		} else {
			rc = sqlite3_bind_blob(insert_stmt,5,&file->mdContext,sizeof(SHA512_Context),NULL);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind SHA512 context");
			status = FAILURE;
		}
	}

	/* Execute prepared statement */
	if(SUCCESS == status)
	{
		rc = sqlite3_step(insert_stmt);

		if(rc != SQLITE_DONE)
		{
			log_sqlite_error(config->db,rc,NULL,"Insert statement failed");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Reflect changes in global */
		config->db_primary_file_modified = true;

	}

	sqlite3_finalize(insert_stmt);

	provide(status);
}
