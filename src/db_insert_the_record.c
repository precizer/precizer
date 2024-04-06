#include "precizer.h"

/**
 *
 * @brief Insert the record into db.
 * @details Insert information about the file, its
 * metadata and checksum into the database
 *
 */
Return db_insert_the_record
(
#if 0 // Old multiPATH solution
	const sqlite3_int64 *path_prefix_index,
#endif
	const char *relative_path,
	const sqlite3_int64 *offset,
	const unsigned char *sha512,
	const struct stat *stat,
	const SHA512_Context *mdContext
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Don't do anything in case of --dry_run
	if(config->dry_run == true)
	{
		return(status);
	}

	/* Insert to DB */
	int rc = 0;

#if 0 // Old multiPATH solution
	const char *insert_sql = "INSERT INTO files (offset,path_prefix_index,relative_path,sha512,stat,mdContext) VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
#endif
	const char *insert_sql = "INSERT INTO files (offset,relative_path,sha512,stat,mdContext) VALUES (?1, ?2, ?3, ?4, ?5);";
	sqlite3_stmt *insert_stmt = NULL;

	/* Create SQL statement. Prepare to write */
	rc = sqlite3_prepare_v2(config->db, insert_sql, -1, &insert_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare insert statement %s (%i): %s\n", insert_sql, rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_null(insert_stmt, 1);
	} else {
		rc = sqlite3_bind_int64(insert_stmt, 1, *offset);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

#if 0 // Old multiPATH solution
	rc = sqlite3_bind_int64(insert_stmt, 2, *path_prefix_index);
	if(SQLITE_OK != rc) {
		printf("Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
#endif

	rc = sqlite3_bind_text(insert_stmt, 2, relative_path, (int)strlen(relative_path), NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_blob(insert_stmt, 3, sha512, SHA512_DIGEST_LENGTH, NULL);
	} else {
		rc = sqlite3_bind_null(insert_stmt, 3);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_blob(insert_stmt, 4, stat, sizeof(struct stat), NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	if(*offset == 0){
		rc = sqlite3_bind_null(insert_stmt, 5);
	} else {
		rc = sqlite3_bind_blob(insert_stmt, 5, mdContext, sizeof(SHA512_Context), NULL);
	}
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in insert (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/* Execute SQL statement */
	if(sqlite3_step(insert_stmt) != SQLITE_DONE)
	{
		slog(false,"Insert statement didn't return DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	sqlite3_finalize(insert_stmt);

	return(status);
}
