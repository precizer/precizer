#include "precizer.h"

/**
 *
 * The structure Config where all runtime settings will be stored.
 * Initialization the structure elements by zerro.
 *
 */
void init_config(void){

	// Fill out with zerroes
	memset(config,0,sizeof(Config));

	// Max available size of a path
	config->running_dir_size = 0;

	// Total size of all scanned files
	config->total_size_in_bytes = 0;

	// Absolute path name of the working directory
	// A directory where the program was executed
	config->running_dir = NULL;

	// Show progress bar
	config->progress = false;

	// Force update of the database
	config->force = false;

	// Additional output for debugging
	config->verbose = false;

	// Force update of the database with new,
	// changed or deleted files. This is special
	// protection against accidental deletion of
	// information from the database.
	config->update = false;

	// Parameter to compare database
	config->compare = false;

	// An array of paths to traverse
	config->paths = NULL;

	// The pointer to the main database
	config->db = NULL;

	/// The flags parameter to sqlite3_open_v2()
	/// must include, at a minimum, one of the
	/// following flag combinations:
	///   - SQLITE_OPEN_READONLY
	///   - SQLITE_OPEN_READWRITE
	///   - SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
	///   - SQLITE_OPEN_MEMORY
	/// Default value: RO
	config->sqlite_open_flag = SQLITE_OPEN_READONLY;

	// The path of DB file
	config->db_file_path = NULL;

	// The name of DB file
	config->db_file_name = NULL;

	// Pointers to the array with database paths
	config->db_file_paths = NULL;

	// Pointers to the array with database file names
	config->db_file_names = NULL;

	/// Allow or disallow database table
	/// initialization. True by default
	config->db_initialize_tables = true;

	// The flag means that the DB already exists
	// and not empty
	config->db_contains_data = false;

	// Must be specified additionally in order
	// to remove from the database mention of
	// files that matches the regular expression
	// passed through the ignore option(s)
	// This is special protection against accidental
	// deletion of information from the database.
	config->db_clean_ignored = false;

	/// Select database validation level: 'quick' (default)
	/// for basic structure check, 'full' for comprehensive
	/// integrity verification
	config->db_check_level = QUICK;

	// Flag that reflects the presence of any changes
	// since the last research
	config->something_has_been_changed = false;

	// Recursion depth limit. The depth of the traversal,
	// numbered from 0 to N, where a file could be found.
	// Representing the maximum of the starting
	// point (from root) of the traversal.
	// The root itself is numbered 0
	config->maxdepth = -1;

	// Ignore those relative paths
	// The string array of PCRE2 regular expressions
	config->ignore = NULL;

	// Include those relative paths even if
	// they were excluded via the --ignore option
	// The string array of PCRE2 regular expressions
	config->include = NULL;

	// Perform a trial run with no changes made
	config->dry_run = false;

	// Define the comparison string
	const char *compare_string = "true";

	// Retrieve the value of the "TESTING" environment variable,
	// Check up if the environment variable TESTING exists
	// and if it match to "true" display ONLY testing
	// messages for System Testing purposes.
	const char *env_var = getenv("TESTING");

	// Check if it exists and compare it to "true"
	if(env_var != NULL && strncmp(env_var,compare_string,sizeof(compare_string) - 1) == 0)
	{
		// Global variable
		rational_logger_mode = TESTING;
	}

	slog(TRACE,"Configuration initialized\n");
}
