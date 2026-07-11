#include "precizer.h"

/**
 * @brief Save the current per-file state into the database
 *
 * This function is the common save point for one file row. It writes the file's
 * current metadata, SHA512 digest or partial SHA512 resume state, and row status
 * to SQLite. It chooses whether to insert a new row or update an existing row by
 * looking at @p path_known
 *
 * The helper is used by both final file saves and silent SHA512 checkpoints. A
 * checkpoint may be the first time a new file is written to SQLite during the
 * current traversal. In that case the helper inserts the row, stores the new row
 * ID in @p file, and updates @p path_known so later saves for the same file
 * update that row instead of inserting a duplicate. When @p path_known is already
 * true, the helper updates the existing row directly
 *
 * @p mark_visible_change controls user-facing reporting. Final saves pass true
 * so show_file() can say whether the file was inserted or updated. Silent
 * checkpoints pass false because they are only recovery points for interrupted
 * hashing and should not appear as ordinary traversal changes
 *
 * @param[in] relative_path File path relative to the traversal root
 * @param[in,out] file Per-file state to persist and annotate
 * @param[in,out] path_known True when the row currently exists in SQLite
 * @param[in] mark_visible_change True for the final save reported to the user
 * @return SUCCESS when the row was saved, otherwise FAILURE
 */
Return db_save_file_record(
	const memory *relative_path,
	File         *file,
	bool         *path_known,
	const bool   mark_visible_change)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	if(relative_path == NULL || file == NULL || file->db == NULL || path_known == NULL)
	{
		provide(FAILURE);
	}

	if(config->dry_run == true && mark_visible_change == false)
	{
		provide(status);
	}

	/*
	 * Remember whether the row existed before this file started processing.
	 * A silent checkpoint can insert a new row before the final save, but the
	 * user-facing result should still say "inserted", not "updated"
	 */
	const bool row_existed_before_processing = file->db->relative_path_was_in_db_before_processing == true;

	/*
	 * Save into the row that is known for the current traversal state.
	 * Existing paths are updated by ID. New paths are inserted once, then their
	 * generated ID and path_known flag are kept so later checkpoints or the final
	 * save update the same row instead of inserting another one
	 */
	if(*path_known == true)
	{
		status = db_update_the_record_by_id(file);

	} else {
		status = db_insert_the_record(relative_path,file);

		if(TRIUMPH & status)
		{
			if(config->dry_run == false)
			{
				file->db->ID = sqlite3_last_insert_rowid(config->db);
				*path_known = true;
			}
		}
	}

	/*
	 * Mark only visible saves for the traversal report.
	 * Silent checkpoints may write the same row earlier, but they must not make
	 * the file look updated to the user. The original row state decides whether
	 * the final visible result is an insertion or an update
	 */
	if((TRIUMPH & status) && mark_visible_change == true)
	{
		if(row_existed_before_processing == true)
		{
			file->db_record_updated = true;

		} else {
			file->new_db_record_inserted = true;
		}
	}

	provide(status);
}
