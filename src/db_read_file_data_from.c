#include "precizer.h"

/**
 * @brief Read one file row from the database into file->db
 *
 * Loads the saved row for @p relative_path into the DBrow attached to @p file.
 * If the path is absent from the database, the attached row stays zeroed and
 * relative_path_was_in_db_before_processing remains false
 *
 * @param[in,out] file Per-file state whose attached DB row receives the loaded values
 * @param[in] relative_path Relative path descriptor looked up in the files table
 * @return SUCCESS on a completed lookup, otherwise FAILURE
 */
Return db_read_file_data_from(
	File *file,
#if 0 // Disabled multi-root path index implementation
	const sqlite3_int64 *path_prefix_index,
#endif
	const memory *relative_path)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Read from the database */
	sqlite3_stmt *select_stmt = NULL;
	int rc;
	// Convenience alias for the attached DB row that receives the loaded values
	DBrow *dbrow = file->db;

	/* Create SQL statement */
#if 0 // Disabled multi-root path index implementation
	const char *select_sql = "SELECT ID,offset,stat,mdContext,sha512 FROM files WHERE path_prefix_index = ?1 and relative_path = ?2;";
#else
	const char *select_sql = "SELECT ID,offset,stat,mdContext,sha512 FROM files WHERE relative_path = ?1;";
#endif
	rc = sqlite3_prepare_v2(config->db,select_sql,-1,&select_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare select statement %s",select_sql);
		status = FAILURE;
	}

#if 0 // Disabled multi-root path index implementation
	rc = sqlite3_bind_int64(select_stmt,1,*path_prefix_index);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Error binding value in select");
		status = FAILURE;
	}
#endif
	size_t relative_path_length;

	if(SUCCESS & status)
	{
		status = m_string_length(relative_path,&relative_path_length);
	}

	if(SUCCESS & status)
	{
		rc = sqlite3_bind_text(select_stmt,1,m_text(relative_path),(int)relative_path_length,NULL);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in select");
			status = FAILURE;
		}
	}

	if(SUCCESS & status)
	{
		while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
		{
			dbrow->ID = sqlite3_column_int64(select_stmt,0);
			dbrow->saved_offset = sqlite3_column_int64(select_stmt,1);
			const CmpctStat *get_stat = sqlite3_column_blob(select_stmt,2);

			if(get_stat != NULL)
			{
				memcpy(&dbrow->saved_stat,get_stat,sizeof(CmpctStat));
			}
			const SHA512_Context *get_mdContext = sqlite3_column_blob(select_stmt,3);

			if(get_mdContext != NULL)
			{
				memcpy(&dbrow->saved_mdContext,get_mdContext,sizeof(SHA512_Context));
			}

			const unsigned char *get_sha512 = sqlite3_column_blob(select_stmt,4);

			if(get_sha512 != NULL)
			{
				memcpy(&dbrow->sha512,get_sha512,SHA512_DIGEST_LENGTH);
			}

			dbrow->relative_path_was_in_db_before_processing = true;
		}

		if(SQLITE_DONE != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Select statement didn't finish with DONE");
			status = FAILURE;
		}
	}
	rc = sqlite3_finalize(select_stmt);

	if(SUCCESS & status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize select statement");
		status = FAILURE;
	}

	provide(status);
}
