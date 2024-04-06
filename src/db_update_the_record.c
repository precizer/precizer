#include "precizer.h"

/**
 *
 * @brief Update the record against db.
 * @details Update information about the file, its
 * metadata and checksum against the database
 *
 */
Return db_update_the_record
(
	const sqlite3_int64 *ID,
	const sqlite3_int64 *offset,
	const unsigned char *sha512,
	const struct stat *stat,
	const SHA512_Context *mdContext
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do enything in case of --dry_run
	if(config->dry_run == true)
	{
		return(status);
	}

	/* Update record in DB */
	int rc = 0;

	sqlite3_stmt *update_stmt = NULL;
	const char *update_sql = "UPDATE files SET offset = ?1, sha512 = ?2, stat = ?3, mdContext = ?4 WHERE ID = ?5;";

	/* Create SQL statement. Prepare to write */
	rc = sqlite3_prepare_v2(config->db, update_sql, -1, &update_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare update statement %s (%i): %s\n", update_sql, rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_null(update_stmt, 1);
	} else {
		rc = sqlite3_bind_int64(update_stmt, 1, *offset);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in update (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_blob(update_stmt, 2, sha512, SHA512_DIGEST_LENGTH, NULL);
	} else {
		rc = sqlite3_bind_null(update_stmt, 2);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in update (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_blob(update_stmt, 3, stat, sizeof(struct stat), NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in update (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_null(update_stmt, 4);
	} else {
		rc = sqlite3_bind_blob(update_stmt, 4, mdContext, sizeof(SHA512_Context), NULL);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in update (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_int64(update_stmt, 5, *ID);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in update (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/* Execute SQL statement */
	if(sqlite3_step(update_stmt) != SQLITE_DONE)
	{
		slog(false,"Update statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(update_stmt);

	return(status);
}
