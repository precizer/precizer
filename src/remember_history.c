#include "precizer.h"

/**
 *
 * @brief Store a formatted log line in the TEMP remember_history table.
 *
 * @details The table is created in db_init(). If the database is unavailable,
 *          the function returns without inserting anything.
 *
 * @note The message buffer may be non-null-terminated; line_len is used
 *       to bound the insert.
 *
 */
void rational_remember(
	const char *message,
	const int  line_len)
{
	if(message == NULL)
	{
		return;
	}

	if(line_len <= 0)
	{
		return;
	}

	sqlite3_stmt *lookup_stmt = NULL;
	sqlite3_stmt *write_stmt = NULL;
	sqlite3_int64 last_id = 0;
	bool append_to_last = false;

	const char *select_sql =
	        "SELECT id, message FROM temp.remember_history "
	        "ORDER BY id DESC LIMIT 1;";

	int rc = sqlite3_prepare_v2(config->db,select_sql,-1,&lookup_stmt,NULL);

	if(rc != SQLITE_OK)
	{
		log_sqlite_error(config->db,rc,NULL,
			"Failed to prepare remember_history lookup");
	} else {
		rc = sqlite3_step(lookup_stmt);

		if(rc == SQLITE_ROW)
		{
			const unsigned char *text = sqlite3_column_text(lookup_stmt,1);
			const int text_len = sqlite3_column_bytes(lookup_stmt,1);

			if(text != NULL && text_len > 0 && text[text_len - 1] != '\n')
			{
				last_id = sqlite3_column_int64(lookup_stmt,0);
				append_to_last = true;
			}
		}
	}

	sqlite3_finalize(lookup_stmt);

	if(append_to_last)
	{
		const char *update_sql =
		        "UPDATE temp.remember_history "
		        "SET message = message || ?1 WHERE id = ?2;";

		rc = sqlite3_prepare_v2(config->db,update_sql,-1,&write_stmt,NULL);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,
				"Failed to prepare remember_history update");
			sqlite3_finalize(write_stmt);
			return;
		}

		rc = sqlite3_bind_text(write_stmt,1,message,line_len,SQLITE_TRANSIENT);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,
				"Failed to bind remember_history update");
			sqlite3_finalize(write_stmt);
			return;
		}

		rc = sqlite3_bind_int64(write_stmt,2,last_id);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,
				"Failed to bind remember_history id");
			sqlite3_finalize(write_stmt);
			return;
		}
	} else {
		const char *insert_sql =
		        "INSERT INTO temp.remember_history (message) VALUES (?1);";

		rc = sqlite3_prepare_v2(config->db,insert_sql,-1,&write_stmt,NULL);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,
				"Failed to prepare remember_history insert");
			sqlite3_finalize(write_stmt);
			return;
		}

		rc = sqlite3_bind_text(write_stmt,1,message,line_len,SQLITE_TRANSIENT);
	}

	if(rc != SQLITE_OK)
	{
		log_sqlite_error(config->db,rc,NULL,
			"Failed to bind remember_history message");
		sqlite3_finalize(write_stmt);
		return;
	}

	rc = sqlite3_step(write_stmt);

	if(rc != SQLITE_DONE)
	{
		if(append_to_last)
		{
			log_sqlite_error(config->db,rc,NULL,
				"Failed to update remember_history record");
		} else {
			log_sqlite_error(config->db,rc,NULL,
				"Failed to insert remember_history record");
		}
	}

	sqlite3_finalize(write_stmt);

	if(rc != SQLITE_DONE)
	{
		return;
	}
}
