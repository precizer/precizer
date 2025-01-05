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
 * @param[in] offset       File offset value, 0 indicates no offset
 * @param[in] sha512       SHA512 checksum of the file, NULL if offset is non-zero
 * @param[in] stat         File metadata structure
 * @param[in] mdContext    SHA512 context structure, NULL if offset is zero
 *
 * @return Return status code:
 *         - SUCCESS: Record inserted successfully
 *         - FAILURE: Error occurred during insertion
 *
 * @note In dry run mode, the function returns SUCCESS without modifying the database
 */
Return db_insert_the_record(
	const char           *relative_path,
	const sqlite3_int64  *offset,
	const unsigned char  *sha512,
	const CmpctStat      *stat,
	const SHA512_Context *mdContext
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Skip database operations in dry run mode --dry-run */
	if(config->dry_run == true)
	{
		return(status);
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
		slog(ERROR,"Failed to prepare insert statement %s (%i): %s\n",insert_sql,rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/* Bind offset value */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_null(insert_stmt,1);
		} else {
			rc = sqlite3_bind_int64(insert_stmt,1,*offset);
		}

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to bind offset value (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Bind relative path */
	if(SUCCESS == status)
	{
		rc = sqlite3_bind_text(insert_stmt,2,relative_path,(int)strlen(relative_path),NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to bind relative path (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_blob(insert_stmt,3,sha512,SHA512_DIGEST_LENGTH,NULL);
		} else {
			rc = sqlite3_bind_null(insert_stmt,3);
		}

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to bind SHA512 checksum (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Copy and bind file metadata */
	if(SUCCESS == status)
	{
		if(SUCCESS == status)
		{
			rc = sqlite3_bind_blob(insert_stmt,4,stat,sizeof(CmpctStat),NULL);

			if(SQLITE_OK != rc)
			{
				slog(ERROR,"Failed to bind file metadata (%i): %s\n",rc,sqlite3_errmsg(config->db));
				status = FAILURE;
			}
		}
	}

	/* Bind SHA512 context */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_null(insert_stmt,5);
		} else {
			rc = sqlite3_bind_blob(insert_stmt,5,mdContext,sizeof(SHA512_Context),NULL);
		}

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to bind SHA512 context (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Execute prepared statement */
	if(SUCCESS == status)
	{
		if(sqlite3_step(insert_stmt) != SQLITE_DONE)
		{
			slog(ERROR,"Insert statement failed (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	sqlite3_finalize(insert_stmt);

	return(status);
}
