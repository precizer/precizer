#include "precizer.h"

/**
 * @brief Update the record in database
 * @details Update information about the file, its
 * metadata and checksum against the database
 *
 * @param[in] ID Database record identifier
 * @param[in] offset File offset value
 * @param[in] sha512 SHA512 checksum
 * @param[in] stat File metadata structure
 * @param[in] mdContext SHA512 context
 *
 * @return Return status code:
 *         - SUCCESS: Record updated successfully
 *         - FAILURE: Error occurred during update
 *
 * @note In dry run mode, the function returns SUCCESS without modifying the database
 */
Return db_update_the_record_by_id(
	const sqlite3_int64  *ID,
	const sqlite3_int64  *offset,
	const unsigned char  *sha512,
	const CmpctStat      *stat,
	const SHA512_Context *mdContext,
	const bool           *zero_size_file,
	const bool           *wrong_file_type)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/* Skip database operations in dry run mode --dry-run */
	if(config->dry_run == true)
	{
		provide(status);
	}

	int rc = 0;

	sqlite3_stmt *update_stmt = NULL;

	const char *update_sql = "UPDATE files SET offset = ?1,sha512 = ?2,stat = ?3,mdContext = ?4 WHERE ID = ?5;";

	/* Create SQL statement. Prepare to write */
	rc = sqlite3_prepare_v2(config->db,update_sql,-1,&update_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to prepare update statement %s",update_sql);
		status = FAILURE;
	}

	/* Bind offset value */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_null(update_stmt,1);
		} else {
			rc = sqlite3_bind_int64(update_stmt,1,*offset);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind offset value in update");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(update_stmt,5,*ID);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in update");
			status = FAILURE;
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS == status)
	{
		if(*offset == 0 && *zero_size_file == false && *wrong_file_type == false)
		{
			rc = sqlite3_bind_blob(update_stmt,2,sha512,SHA512_DIGEST_LENGTH,NULL);
		} else {
			rc = sqlite3_bind_null(update_stmt,2);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind sha512 hash value in update");
			status = FAILURE;
		}
	}

	/* Copy and bind file metadata */
	if(SUCCESS == status)
	{
		rc = sqlite3_bind_blob(update_stmt,3,stat,sizeof(CmpctStat),NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in update");
			status = FAILURE;
		}
	}

	/* Bind SHA512 context */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_null(update_stmt,4);
		} else {
			rc = sqlite3_bind_blob(update_stmt,4,mdContext,sizeof(SHA512_Context),NULL);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in update");
			status = FAILURE;
		}
	}

	/* Execute prepared statement */
	if(SUCCESS == status)
	{
		rc = sqlite3_step(update_stmt);

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Update statement failed");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		/* Reflect changes in global */
		config->db_primary_file_modified = true;
	}

	sqlite3_finalize(update_stmt);

	provide(status);
}
