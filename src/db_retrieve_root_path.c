#include "precizer.h"

/**
 * @brief Retrieve the required root path prefix from the paths table
 *
 * @details The function expects `paths.prefix` to contain one non-empty string
 * that represents the initial path stored in the database. Returning `SUCCESS`
 * means that this prefix was fetched and copied into `root_path`
 *
 * @param[out] root_path Non-NULL descriptor that receives the non-empty prefix string
 *
 * @return Return status code:
 *         - SUCCESS: The non-empty prefix was fetched and copied
 *         - FAILURE: Validation failed, the required prefix is missing or empty, or the query failed
 */
Return db_retrieve_root_path(memory *root_path)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	sqlite3_stmt *prefix_stmt = NULL;

	const char *prefix_sql = "SELECT prefix FROM paths LIMIT 1;";

	int rc = sqlite3_prepare_v2(config->db,prefix_sql,-1,&prefix_stmt,NULL);

	if(SQLITE_OK != rc)
	{
		log_sqlite_error(config->db,rc,NULL,"Can't prepare prefix select statement");
		status = FAILURE;
	}

	if(SUCCESS == status)
	{
		rc = sqlite3_step(prefix_stmt);

		if(SQLITE_ROW == rc)
		{
			const char *prefix = (const char *)sqlite3_column_text(prefix_stmt,0);

			if(prefix != NULL)
			{
				size_t prefix_length = (size_t)sqlite3_column_bytes(prefix_stmt,0);

				if(prefix_length > 0U)
				{
					/*
					 * sqlite3_column_bytes returns the visible string length.
					 * Copy one extra byte so the stored terminator is preserved
					 */
					status = m_copy_fixed_string(root_path,prefix_length + 1U,prefix);

				} else {
					slog(ERROR,"The paths table contains an empty root path prefix\n");
					status = FAILURE;
				}

			} else {
				slog(ERROR,"The paths table returned a NULL root path prefix\n");
				status = FAILURE;
			}

		} else if(SQLITE_DONE == rc){
			slog(ERROR,"The required root path prefix is missing from the paths table\n");
			status = FAILURE;

		} else {
			log_sqlite_error(config->db,rc,NULL,"Can't fetch path prefix");
			status = FAILURE;
		}
	}

	sqlite3_finalize(prefix_stmt);

	provide(status);
}
