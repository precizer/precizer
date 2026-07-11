#include "precizer.h"

/**
 * @brief Drop a file record from the database by its unique ID
 *
 * @details In --dry-run mode the function verifies the target row with a
 * `SELECT` instead of executing `DELETE`. Policy decisions and user-visible
 * logging are handled by the caller.
 * A successful real `DELETE` marks the primary database as modified
 *
 * @param[in] ID Unique row ID from the `files` table
 * @return Return status code
 */
Return db_delete_the_record_by_id(const sqlite_int64 *ID)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3_stmt *delete_stmt = NULL;
	int rc = 0;

	const char *sql = "DELETE FROM files WHERE ID=?1;";

	// Don't do anything in case of --dry-run
	if(config->dry_run == true)
	{
		sql = "SELECT ID FROM files WHERE ID=?1;";
	}

	rc = sqlite3_prepare_v2(config->db,sql,-1,&delete_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare delete statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_bind_int64(delete_stmt,1,*ID);

		if(SQLITE_OK != rc)
		{
			log_sqlite_error(config->db,rc,NULL,"Error binding value in delete");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		int sql_return = SQLITE_DONE;

		/*
		 * In normal mode sqlite3_step() executes DELETE and must finish with SQLITE_DONE.
		 * In --dry-run mode the SQL text above was replaced with SELECT, so the first
		 * sqlite3_step() must yield SQLITE_ROW for the matching record instead
		 */
		if(config->dry_run == true)
		{
			sql_return = SQLITE_ROW;
		}

		/* Execute SQL statement */
		rc = sqlite3_step(delete_stmt);

		/*
		 * One comparison handles both modes:
		 * - normal run: DELETE must finish with SQLITE_DONE
		 * - --dry-run: SELECT must return SQLITE_ROW for the existing target record
		 */
		if(rc != sql_return)
		{
			log_sqlite_error(config->db,rc,NULL,"Delete statement didn't return right code %d",sql_return);
			status = FAILURE;
		}
	}

	if(SUCCESS == status && config->dry_run == false)
	{
		/* Reflect changes in global */
		config->db_primary_file_modified = true;
	}

	rc = sqlite3_finalize(delete_stmt);

	if(SUCCESS == status && SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Failed to finalize delete statement");
		status = FAILURE;
	}

	provide(status);
}
