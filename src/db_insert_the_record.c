/**
 * @file db_insert_the_record.c
 * @brief Insert new file rows into the SQLite database
 * @details Contains the implementation that binds relative path, checksum, offset,
 *          and metadata fields for a new files table record
 */

#include "precizer.h"

/**
 * @brief Insert one file record into the database
 *
 * Binds the relative path, checksum offset, SHA512 digest, compact stat payload,
 * and saved hashing context for one files-table row.
 * Some bound fields may be NULL when the current file state does not have data for them
 *
 * @param[in] relative_path Descriptor containing the file path relative to the traversal root
 * @param[in] file Per-file state object used to bind the row payload
 * @return SUCCESS when the record was handled cleanly, otherwise FAILURE
 *
 * @note In dry-run mode, the function returns SUCCESS without modifying the database
 */
Return db_insert_the_record(
	const memory *relative_path,
	const File   *file)
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

#if 0 // Disabled multi-root path index implementation
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
	if(SUCCESS & status)
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
	if(SUCCESS & status)
	{
		size_t relative_path_length;

		status = m_string_length(relative_path,&relative_path_length);

		if(SUCCESS & status)
		{
			rc = sqlite3_bind_text(insert_stmt,2,m_text(relative_path),(int)relative_path_length,NULL);

			if(SQLITE_OK != rc)
			{
				log_sqlite_error(config->db,rc,NULL,"Failed to bind relative path");
				status = FAILURE;
			}
		}
	}

	/* Bind SHA512 checksum */
	if(SUCCESS & status)
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
	if(SUCCESS & status)
	{
		rc = sqlite3_bind_blob(insert_stmt,4,&file->stat,sizeof(CmpctStat),NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Failed to bind file metadata");
			status = FAILURE;
		}
	}

	/* Bind SHA512 context */
	if(SUCCESS & status)
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
	if(SUCCESS & status)
	{
		rc = sqlite3_step(insert_stmt);

		if(rc != SQLITE_DONE)
		{
			log_sqlite_error(config->db,rc,NULL,"Insert statement failed");
			status = FAILURE;
		}
	}

	if(SUCCESS & status)
	{
		/* Reflect changes in global */
		config->db_primary_file_modified = true;

	}

	rc = sqlite3_finalize(insert_stmt);

	if(SUCCESS & status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize insert statement");
		status = FAILURE;
	}

	provide(status);
}
