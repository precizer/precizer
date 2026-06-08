#include "precizer.h"
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/**
 *
 * Initialize SQLite database
 *
 */
Return db_init(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/* Interrupt the function smoothly */
	/* Interrupt when Ctrl+C */
	if(global_interrupt_flag == true)
	{
		provide(status);
	}

	// SQL request result
	int rc;

	/* Open database */

	const char *db_file_path = confstr(db_primary_file_path);

	if(config->sqlite_open_flag == SQL_DRY_RUN_MODE)
	{
		db_file_path = ":memory:";
		config->db_primary_path_is_memory = true;
		config->sqlite_open_flag = SQLITE_OPEN_READWRITE;
		slog(TRACE,"Dry Run mode was activated. In-memory database will be used to simulate activity.\n");
	}

	if(SQLITE_OK == (rc = sqlite3_open_v2(db_file_path,&config->db,config->sqlite_open_flag,NULL)))
	{
		slog(TRACE,"Successfully opened database %s\n",confstr(db_file_name));
	} else if(config->compare != true){
		log_sqlite_error(config->db,
			rc,
			NULL,
			"Can't open database %s",
			confstr(db_primary_file_path));
		sqlite3_close(config->db);
		config->db = NULL;
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
			const char *sql =
			        "PRAGMA foreign_keys=OFF;"
			        "PRAGMA journal_mode=DELETE;"        // Use DELETE journal to avoid WAL artifacts
			        "PRAGMA page_size=4096;"            // Set page size to 4KB (default, but explicit for clarity)
			        "PRAGMA cache_size=-8192;"          // Use 8MB of memory for caching (negative value = KB)
			        "PRAGMA synchronous=NORMAL;"        // Balance speed and safety (NORMAL = fsync only for checkpoints)
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
			        "REPLACE INTO metadata (db_version) VALUES (" TOSTRING(CURRENT_DB_VERSION) ");"
			        "COMMIT;";
#endif

			/* Full runtime path is stored in the table 'paths' */
			const char *sql =
			        "PRAGMA foreign_keys=OFF;"
			        "PRAGMA journal_mode=DELETE;"        // Use DELETE journal to avoid WAL artifacts
			        "PRAGMA page_size=4096;"            // Set page size to 4KB (default, but explicit for clarity)
			        "PRAGMA cache_size=-8192;"          // Use 8MB of memory for caching (negative value = KB)
			        "PRAGMA synchronous=NORMAL;"        // Balance speed and safety (NORMAL = fsync only for checkpoints)
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
			        "REPLACE INTO metadata (db_version) VALUES (" TOSTRING(CURRENT_DB_VERSION) ");"
			        "COMMIT;";

			/* Execute SQL statement */
			rc = sqlite3_exec(config->db,sql,NULL,NULL,NULL);

			if(rc == SQLITE_OK)
			{
				slog(TRACE,"The primary database and tables have been successfully initialized\n");
			} else {
				log_sqlite_error(config->db,rc,NULL,"Can't execute table initialization");
				status = FAILURE;
			}
		}
	} else {
		slog(TRACE,"The primary database and tables do not require initialization\n");
	}

	if(SUCCESS == status)
	{
		// Tune the DB performance
		const char *pragma_sql = NULL;

		if(config->sqlite_open_flag == SQLITE_OPEN_READONLY)
		{
			// Read-only mode
			pragma_sql =
			        "PRAGMA synchronous=OFF;"            // Disable fsync to speed up read-only access
			        "PRAGMA cache_size=-8192;"           // Increased cache to 8MB
			        "PRAGMA temp_store=MEMORY;"          // Keep temporary data in RAM
			        "PRAGMA mmap_size=30000000000;"      // Using memory-mapped I/O
			        "PRAGMA locking_mode=EXCLUSIVE;"     // Hold exclusive locks for the session
			        "PRAGMA strict=ON;";                  // Enforce STRICT table schema validation
		} else {
			// Read-write mode
			pragma_sql =
			        "PRAGMA journal_mode=DELETE; "       // Use DELETE journal
			        "PRAGMA cache_size=-8192; "          // Use 8MB of memory for caching (negative value = KB)
			        "PRAGMA synchronous=NORMAL; "        // Balance speed and safety (NORMAL = fsync only for checkpoints)
			        "PRAGMA temp_store=MEMORY; "         // Store temporary tables in memory (not on disk)
			        "PRAGMA strict=ON;"                  // Enforce STRICT table schema validation
			        "PRAGMA locking_mode=EXCLUSIVE;";    // Hold exclusive locks for the session
		}

		// Set SQLite pragmas
		rc = sqlite3_exec(config->db,pragma_sql,NULL,NULL,NULL);

		if(rc == SQLITE_OK)
		{
			slog(TRACE,"The primary database named %s is ready for operations\n",confstr(db_file_name));
		} else {
			log_sqlite_error(config->db,rc,NULL,"Can't execute pragma setup");
			status = FAILURE;
		}
	}

	if(SUCCESS == status)
	{
		const char *remember_history_sql =
		        "CREATE TEMP TABLE IF NOT EXISTS remember_history ("
		        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
		        "message TEXT NOT NULL"
		        ");";

		rc = sqlite3_exec(config->db,remember_history_sql,NULL,NULL,NULL);

		if(rc != SQLITE_OK)
		{
			log_sqlite_error(config->db,rc,NULL,"Can't create TEMP remember_history table");
		}

		if(config->compare != true)
		{
			const char *db_runtime_paths =
			        "CREATE TEMP TABLE IF NOT EXISTS the_path_id_does_not_exists"
			        "(path_id INTEGER UNIQUE NOT NULL);";

			rc = sqlite3_exec(config->db,db_runtime_paths,NULL,NULL,NULL);

			if(rc == SQLITE_OK)
			{
				slog(TRACE,"The TEMP table the_path_id_does_not_exists is ready for runtime checks\n");
			} else {
				log_sqlite_error(config->db,rc,NULL,"Can't create runtime TEMP table");
				status = FAILURE;
			}
		}
	}

	slog(TRACE,"Database initialization process completed\n");

	provide(status);
}
