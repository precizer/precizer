#include "precizer.h"

/**
 * @brief Update the record in database
 * @details Update information about the file, its
 * metadata and checksum against the database
 *
 * @param[in] file Per-file state object. Uses db->ID when selecting the row
 *                 and uses checksum_offset, sha512, stat, mdContext,
 *                 zero_size_file, and wrong_file_type when binding the updated
 *                 row values
 *
 * @return Return status code:
 *         - SUCCESS: Record updated successfully
 *         - FAILURE: Error occurred during update
 *
 * @note In dry run mode, the function returns SUCCESS without modifying the database
 */
Return db_update_the_record_by_id(const File *file)
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
		if(file->checksum_offset == 0)
		{
			rc = sqlite3_bind_null(update_stmt,1);
		} else {
			rc = sqlite3_bind_int64(update_stmt,1,file->checksum_offset);
		}

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind offset value in update");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(update_stmt,5,file->db->ID);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in update");
			status = FAILURE;
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS == status)
	{
		if(file->checksum_offset == 0 && file->zero_size_file == false && file->wrong_file_type == false)
		{
			rc = sqlite3_bind_blob(update_stmt,2,file->sha512,SHA512_DIGEST_LENGTH,NULL);
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
		rc = sqlite3_bind_blob(update_stmt,3,&file->stat,sizeof(CmpctStat),NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in update");
			status = FAILURE;
		}
	}

	/* Bind SHA512 context */
	if(SUCCESS == status)
	{
		if(file->checksum_offset == 0)
		{
			rc = sqlite3_bind_null(update_stmt,4);
		} else {
			rc = sqlite3_bind_blob(update_stmt,4,&file->mdContext,sizeof(SHA512_Context),NULL);
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
