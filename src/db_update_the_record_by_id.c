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

	sqlite3_stmt *update_stmt = NULL;

	const char *update_sql = "UPDATE files SET offset = ?1,sha512 = ?2,stat = ?3,mdContext = ?4 WHERE ID = ?5;";

	/* Create SQL statement. Prepare to write */
	rc = sqlite3_prepare_v2(config->db,update_sql,-1,&update_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		slog(ERROR,"Failed to prepare update statement %s (%i): %s\n",update_sql,rc,sqlite3_errmsg(config->db));
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
			slog(ERROR,"Failed to bind offset value in update (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(update_stmt,5,*ID);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Error binding value in update (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS == status)
	{
		if(*offset == 0)
		{
			rc = sqlite3_bind_blob(update_stmt,2,sha512,SHA512_DIGEST_LENGTH,NULL);
		} else {
			rc = sqlite3_bind_null(update_stmt,2);
		}

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Failed to bind sha512 hash value in update (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Copy and bind file metadata */
	if(SUCCESS == status)
	{
		rc = sqlite3_bind_blob(update_stmt,3,stat,sizeof(CmpctStat),NULL);

		if(SQLITE_OK != rc)
		{
			slog(ERROR,"Error binding value in update (%i): %s\n",rc,sqlite3_errmsg(config->db));
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
			slog(ERROR,"Error binding value in update (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	/* Execute prepared statement */
	if(SUCCESS == status)
	{
		if(sqlite3_step(update_stmt) != SQLITE_DONE)
		{
			slog(ERROR,"Update statement failed (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	sqlite3_finalize(update_stmt);

	return(status);
}
