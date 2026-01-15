#include "precizer.h"

/**
 *
 * @brief Print warnings and errors captured in the TEMP remember_history table.
 *
 * @details The table is created in db_init(). Output is produced only when
 *          --progress is enabled. If the database is unavailable or no rows
 *          are present, the function returns without output.
 *
 * @return SUCCESS on completion, FAILURE if a SQLite operation fails.
 *
 */
Return show_remembered_messages(void)
{
	// Show only with --progress
	if(config->progress != true)
	{
		provide(SUCCESS);
	}

	sqlite3_stmt *stmt = NULL;
	const char *select_sql =
		"SELECT message FROM temp.remember_history ORDER BY id;";

	int rc = sqlite3_prepare_v2(config->db,select_sql,-1,&stmt,NULL);

	if(rc != SQLITE_OK)
	{
		log_sqlite_error(config->db,rc,NULL,
			"Failed to prepare remember_history query");
		provide(FAILURE);
	}

	bool printed_header = false;

	while((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if(!printed_header)
		{
			// Print the header only when at least one row exists.
			slog(EVERY,BOLD "Warnings and errors encountered:" RESET "\n");
			printed_header = true;
		}

		const unsigned char *text = sqlite3_column_text(stmt,0);
		int text_len = sqlite3_column_bytes(stmt,0);

		if(text != NULL && text_len > 0)
		{
			// Use length-bounded output; remember_history messages are not
			// guaranteed to be null-terminated.
			if(text[text_len - 1] == '\n')
			{
				slog(EVERY|UNDECOR,"%.*s",text_len,text);
			} else {
				slog(EVERY|UNDECOR,"%.*s\n",text_len,text);
			}
		}
	}

	if(rc != SQLITE_DONE)
	{
		log_sqlite_error(config->db,rc,NULL,
			"Failed to read remember_history");
		sqlite3_finalize(stmt);
		provide(FAILURE);
	}

	sqlite3_finalize(stmt);

	provide(SUCCESS);
}
