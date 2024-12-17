#include "precizer.h"

/**
 * @brief Gets corresponding string based on provided code
 * @return Pointer to selected string constant
 */
static const char *get_flag_string_by_code(void){
	switch(config->sqlite_open_flag)
	{
		case SQLITE_OPEN_READONLY:
			return "SQLITE_OPEN_READONLY";
			break;
		case SQLITE_OPEN_MEMORY:
			return "SQLITE_OPEN_MEMORY";
			break;
		case SQLITE_OPEN_READWRITE:
			return "SQLITE_OPEN_READWRITE";
			break;
		case SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE:
			return "SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE";
			break;
		case SQL_DRY_RUN_MODE:
			return "SQL_DRY_RUN_MODE";
			break;
		default:
			return "ERROR";
			break;
	}
}

/**
 * @brief Gets corresponding string based on provided code
 * @return Pointer to selected string constant
 */
static const char *get_initialize_string_by_code(void){
	if(config->db_initialize_tables == true)
	{
		return "true";

	} else if(config->db_initialize_tables == false)
	{
		return "false";

	} else {

		return "ERROR!";
	}
}

/**
 *
 * @brief Define the database operation mode
 *
 */
Return db_determine_mode(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Initialize tables of the database or not
	// Default value
	config->db_initialize_tables = true;

	/// The flags parameter to sqlite3_open_v2()
	/// must include, at a minimum, one of the
	/// following flag combinations:
	///   - SQLITE_OPEN_READONLY
	///   - SQLITE_OPEN_READWRITE
	///   - SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE
	///   - SQLITE_OPEN_MEMORY
	/// Default value: RO
	config->sqlite_open_flag = SQLITE_OPEN_READONLY;

	// Update mode is enabled for the existing database
	// The new DB file does NOT need to be created
	if(config->update == true)
	{
		if(config->db_file_exists == false)
		{
			// The database file does NOT exists
			slog(ERROR,"Update mode is only available for existing databases. A brand new database file will not be created if it wasn't created previously\n");
			status = FAILURE;
		} else {
			config->sqlite_open_flag = SQLITE_OPEN_READWRITE;
		}

	} else if(config->compare == true)
	{
		// In-memory database enabled
		config->sqlite_open_flag = SQLITE_OPEN_READWRITE;

	} else {

		if(config->db_file_exists == true)
		{
			// RW mode
			config->sqlite_open_flag = SQLITE_OPEN_READWRITE;

		} else {
			// Regular mode
			config->sqlite_open_flag = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE;

			// The database file does NOT exists
			slog(EVERY,"The primary DB file not yet exists. Brand new database will be created\n");
		}
	}

	if(SUCCESS == status)
	{
		if(config->dry_run == true)
		{
			if(config->db_file_exists == true)
			{
				// The database file exists and will be used in
				// Dry Run mode, but no changes will affect it.
				//
				// This describes a scenario where:
				//
				// * A physical database file exists on the filesystem
				// * The application is running in Dry Run mode
				// * The database file will be read but partly kept in a read-only state
				// * Any modifications during execution will be performed in memory only
				// * The original database file will remain unmodified

				// In Dry Run mode, no changes should be applied to the database
				config->db_initialize_tables = false;

			} else {
				// Dry Run mode is activated and the database file hasn't been created
				// before launching without this mode. In this case, the database will be
				// created in memory and this won't affect any changes on disk.
				//
				// This describes a common database initialization scenario where:
				// * The application is running in Dry Run mode (test/simulation mode)
				// * No physical database file exists on disk from previous non-Dry Run executions
				// * As a result, an in-memory database instance is created temporarily
				// * No disk I/O or persistent storage operations will occur

				// In-memory special flag
				config->sqlite_open_flag = SQL_DRY_RUN_MODE;
			}
		}
	}

	slog(TRACE,"Final value for config->sqlite_open_flag: %s\n",get_flag_string_by_code());
	slog(TRACE,"Final value for config->db_initialize_tables: %s\n",get_initialize_string_by_code());

	slog(TRACE,"DB mode determined\n");

	return(status);
}
