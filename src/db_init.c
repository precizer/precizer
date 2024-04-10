#include "precizer.h"

/**
 *
 * Initialize SQLite database
 *
 */
Return db_init(void)
{
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	int sqlite_open_flag = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

	if(config->compare == true || config->dry_run == true)
	{
		sqlite_open_flag = SQLITE_OPEN_READWRITE;
	}

	if(config->update == false)
	{
		sqlite_open_flag = SQLITE_OPEN_READWRITE;

		// DB file doesn't exists. Brand new DB should be created
		if(SUCCESS != detect_a_path(config->db_file_path,SHOULD_BE_A_FILE))
		{
			sqlite_open_flag = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
		}
	}

	int rc;

	/* Open database */
	if(SQLITE_OK != (rc = sqlite3_open_v2(config->db_file_path, &config->db, sqlite_open_flag, NULL)))
	{
		slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	} else if(config->compare != true){
		slog(true,"Opened database %s successfully\n",config->db_file_name);
	}

	// Don't do anything with default database in cases:
	if(config->dry_run == false || config->compare == false || config->update == false)
	{

#if 0 // Old multiPATH solution
		const char *sql = "PRAGMA foreign_keys=OFF;" \
		                  "BEGIN TRANSACTION;" \
		                  "CREATE TABLE IF NOT EXISTS files("  \
		                  "ID INTEGER PRIMARY KEY NOT NULL," \
		                  "offset INTEGER DEFAULT NULL," \
		                  "path_prefix_index INTEGER NOT NULL," \
		                  "relative_path TEXT NOT NULL," \
		                  "sha512 BLOB DEFAULT NULL," \
		                  "stat BLOB DEFAULT NULL," \
		                  "mdContext BLOB DEFAULT NULL," \
		                  "CONSTRAINT full_path UNIQUE (path_prefix_index, relative_path) ON CONFLICT FAIL);" \
		                  "CREATE INDEX IF NOT EXISTS full_path_ASC ON files (path_prefix_index, relative_path ASC);" \
		                  "CREATE TABLE IF NOT EXISTS paths (" \
		                  "ID INTEGER PRIMARY KEY UNIQUE NOT NULL," \
		                  "prefix TEXT NOT NULL UNIQUE);" \
		                  "COMMIT;";
#endif

		/* Full runtime path is stored in the table 'paths' */
		const char *sql = "PRAGMA foreign_keys=OFF;" \
		                  "BEGIN TRANSACTION;" \
		                  "CREATE TABLE IF NOT EXISTS files("  \
		                  "ID INTEGER PRIMARY KEY NOT NULL," \
		                  "offset INTEGER DEFAULT NULL," \
		                  "relative_path TEXT UNIQUE NOT NULL," \
		                  "sha512 BLOB DEFAULT NULL," \
		                  "stat BLOB DEFAULT NULL," \
		                  "mdContext BLOB DEFAULT NULL);" \
		                  "CREATE UNIQUE INDEX IF NOT EXISTS 'TEXT_ASC' ON 'files' ('relative_path' ASC);" \
		                  "CREATE TABLE IF NOT EXISTS paths (" \
		                  "ID INTEGER PRIMARY KEY UNIQUE NOT NULL," \
		                  "prefix TEXT NOT NULL UNIQUE);" \
		                  "COMMIT;";

		/* Execute SQL statement */
		rc = sqlite3_exec(config->db, sql, NULL, NULL, NULL);
		if(rc!= SQLITE_OK ){
			slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
			status = FAILURE;
		} else {
			slog(true,"The database has been successfully initialized\n");
		}
	}

	// Tune the DB performance
	const char *pragma_sql = "PRAGMA page_size = 4096;" \
	                         "PRAGMA cache_size = 524288;" \
	                         "PRAGMA journal_mode = OFF;" \
	                         "PRAGMA synchronous = OFF;";

	// Set SQLite pragmas
	rc = sqlite3_exec(config->db, pragma_sql, NULL, NULL, NULL);
	if(rc!= SQLITE_OK ){
		slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	const char *inmemory_db = "ATTACH DATABASE ':memory:' AS runtime_paths_id;" \
	                          "CREATE TABLE if not exists runtime_paths_id.the_path_id_does_not_exists" \
	                          "(path_id INTEGER UNIQUE NOT NULL);";

	rc = sqlite3_exec(config->db, inmemory_db, NULL, NULL, NULL);
	if(rc!= SQLITE_OK ){
		slog(false,"Can't execute (%i): %s\n", rc, sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	return(status);
}
