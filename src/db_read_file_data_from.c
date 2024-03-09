#include "precizer.h"

Return db_read_file_data_from
(
	DBrow *dbrow,
	const char *relative_path
){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Clean the structure to prevent reuse;
	memset(dbrow,0,sizeof(DBrow));

	/* Read from SQL */
	sqlite3_stmt *select_stmt = NULL;
	int rc;

	/* Create SQL statement */
	const char *select_sql = "SELECT ID,offset,stat,mdContext FROM files WHERE relative_path = ?1;";
	rc = sqlite3_prepare_v2(config->db, select_sql, -1, &select_stmt, NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Can't prepare select statment %s (%i): %s\n", select_sql, rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	rc = sqlite3_bind_text(select_stmt, 1, relative_path, (int)strlen(relative_path), NULL);
	if(SQLITE_OK != rc) {
		slog(false,"Error binding value in select (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(select_stmt)))
	{
		dbrow->ID = sqlite3_column_int64(select_stmt,0);
		dbrow->saved_offset = sqlite3_column_int64(select_stmt,1);
		const struct stat *get_stat = sqlite3_column_blob(select_stmt,2);
		if(get_stat != NULL){
			memcpy(&dbrow->saved_stat,get_stat,sizeof(struct stat));
		}
		const SHA512_Context *get_mdContext = sqlite3_column_blob(select_stmt,3);
		if(get_mdContext != NULL){
			memcpy(&dbrow->saved_mdContext,get_mdContext,sizeof(SHA512_Context));
		}
		dbrow->relative_path_already_in_db = true;
	}
	if(SQLITE_DONE != rc) {
		slog(false,"Select statement didn't finish with DONE (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}
	sqlite3_finalize(select_stmt);

	return(status);
}
