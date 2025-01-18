#include "precizer.h"
#define DB_RUNTIME_PATHS_ID "runtime_paths_id"

/**
 *
 * Initialize SQLite database
 *
 */
Return db_init(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// SQL request result
	int rc;

	/* Open database */

	const char *db_file_path = config->db_file_path;

	if(config->sqlite_open_flag == SQL_DRY_RUN_MODE)
	{
		db_file_path = ":memory:";
		config->sqlite_open_flag = SQLITE_OPEN_READWRITE;
		slog(TRACE,"Dry Run mode was activated. In-memory database will be used to simulate activity.\n");
	}

	if(SQLITE_OK == (rc = sqlite3_open_v2(db_file_path,&config->db,config->sqlite_open_flag,NULL)))
	{
		slog(TRACE,"Successfully opened database %s\n",config->db_file_name);
	} else if(config->compare != true){
		slog(ERROR,"Can't open database %s (%i): %s\n",config->db_file_path,rc,sqlite3_errmsg(config->db));
		status = FAILURE;
	}

	/**
	 * Allow or disallow database table initialization.
	 * If Dry Run mode is active, this option can be useful
	 * to prevent modification of the existing database.
	 *
	 * Table initialization will be necessary when the database
	 * is used in in-memory mode and is effectively recreated
	 * during the first connection.
	 *
	 * Additionally, database table initialization can be used
	 * to update the database schema during software upgrades.
	 *
	 * This describes several database management scenarios:
	 *
	 * 1. Table Initialization Control
	 *   - Option to enable/disable table schema creation
	 *   - Particularly useful with Dry Run mode to prevent schema changes
	 *   - Acts as a safety mechanism for existing databases
	 * 2. In-Memory Database Scenarios
	 *   - When using in-memory mode, tables need initialization
	 *   - Tables are recreated on first connection
	 *   - No persistent storage between sessions
	 * 3. Schema Migration Use Case
	 *   - Can be used during software updates
	 *   - Allows automatic schema updates
	 *   - Supports database structure evolution with software versions
	 */
	if(config->db_initialize_tables == true)
	{
		if(SUCCESS == status)
		{
#if 0 // Frozen multiPATH feature
			const char *sql = "PRAGMA foreign_keys=OFF;"
				    "PRAGMA strict = ON;"
				    "BEGIN TRANSACTION;"
				    "CREATE TABLE IF NOT EXISTS metadata (db_version INTEGER NOT NULL UNIQUE);"
				    "CREATE TABLE IF NOT EXISTS files("  \
				    "ID INTEGER PRIMARY KEY NOT NULL,"
				    "offset INTEGER DEFAULT NULL,"
				    "path_prefix_index INTEGER NOT NULL,"
				    "relative_path TEXT NOT NULL,"
				    "sha512 BLOB DEFAULT NULL,"
				    "stat BLOB DEFAULT NULL,"
				    "mdContext BLOB DEFAULT NULL,"
				    "CONSTRAINT full_path UNIQUE (path_prefix_index, relative_path) ON CONFLICT FAIL);"
				    "CREATE INDEX IF NOT EXISTS full_path_ASC ON files (path_prefix_index, relative_path ASC);"
				    "CREATE TABLE IF NOT EXISTS paths ("
				    "ID INTEGER PRIMARY KEY UNIQUE NOT NULL,"
				    "prefix TEXT NOT NULL UNIQUE);"
				    "COMMIT;";
#endif

			/* Full runtime path is stored in the table 'paths' */
			const char *sql = "PRAGMA foreign_keys=OFF;"
				    "PRAGMA strict = ON;"
				    "BEGIN TRANSACTION;"
				    "CREATE TABLE IF NOT EXISTS metadata (db_version INTEGER NOT NULL UNIQUE);"
				    "CREATE TABLE IF NOT EXISTS files("  \
				    "ID INTEGER PRIMARY KEY NOT NULL,"
				    "offset INTEGER DEFAULT NULL,"
				    "relative_path TEXT UNIQUE NOT NULL,"
				    "sha512 BLOB DEFAULT NULL,"
				    "stat BLOB DEFAULT NULL,"
				    "mdContext BLOB DEFAULT NULL);"
				    "CREATE UNIQUE INDEX IF NOT EXISTS 'TEXT_ASC' ON 'files' ('relative_path' ASC);"
				    "CREATE TABLE IF NOT EXISTS paths ("
				    "ID INTEGER PRIMARY KEY UNIQUE NOT NULL,"
				    "prefix TEXT NOT NULL UNIQUE);"
				    "COMMIT;";

			/* Execute SQL statement */
			rc = sqlite3_exec(config->db,sql,NULL,NULL,NULL);

			if(rc == SQLITE_OK)
			{
				slog(TRACE,"The primary database and tables have been successfully initialized\n");
			} else {
				slog(ERROR,"Can't execute (%i): %s\n",rc,sqlite3_errmsg(config->db));
				status = FAILURE;
			}
		}
	} else {
		slog(TRACE,"The primary database and tables have NOT been initialized\n");
	}

	if(SUCCESS == status)
	{
		// Tune the DB performance
		const char *pragma_sql = NULL;

		if(config->compare == true)
		{
			// Read-only mode
			pragma_sql = "PRAGMA journal_mode = OFF;"
				"PRAGMA synchronous = OFF;"
				"PRAGMA cache_size = -8000;" // Increased cache to 8MB
				"PRAGMA temp_store = MEMORY;"
				"PRAGMA mmap_size = 30000000000;" // Using memory-mapped I/O
				"PRAGMA page_size = 4096;"
				"PRAGMA locking_mode = EXCLUSIVE;"
				"PRAGMA strict = ON;";
		} else {
			// Read-write mode
			pragma_sql = "PRAGMA page_size = 4096;"
					"PRAGMA strict = ON;"
					"PRAGMA cache_size = -2000;" // 2MB memory cache
					"PRAGMA temp_store = MEMORY;"; // In-memory temporary tables
		}

		// Set SQLite pragmas
		rc = sqlite3_exec(config->db,pragma_sql,NULL,NULL,NULL);

		if(rc == SQLITE_OK)
		{
			slog(TRACE,"The primary database named %s is ready for operations\n",config->db_file_name);
		} else {
			slog(ERROR,"Can't execute (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const char *db_runtime_paths = "ATTACH DATABASE ':memory:' AS " DB_RUNTIME_PATHS_ID ";"
			    "CREATE TABLE if not exists runtime_paths_id.the_path_id_does_not_exists"
			    "(path_id INTEGER UNIQUE NOT NULL);";

		rc = sqlite3_exec(config->db,db_runtime_paths,NULL,NULL,NULL);

		if(rc == SQLITE_OK)
		{
			slog(TRACE,"The in-memory %s database successfully attached to the primary database %s\n",DB_RUNTIME_PATHS_ID,config->db_file_name);
		} else {
			slog(ERROR,"Can't execute (%i): %s\n",rc,sqlite3_errmsg(config->db));
			status = FAILURE;
		}
	}

	slog(TRACE,"Database initialization process completed\n");

	return(status);
}
