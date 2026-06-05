#include "precizer.h"

#if 0
#include <locale.h>
#endif

/**
 * @brief Initialize global runtime configuration with explicit defaults
 *
 * The function clears the shared `Config` object and then initializes every
 * field that needs a non-zero default. Path-like data owned by libmem is
 * prepared as string descriptors or descriptor arrays: `running_dir`,
 * `db_primary_file_path`, `db_file_name`, `roots`, and `db_file_paths`
 *
 * Normal scan roots are collected later in `config->roots`, while `--compare`
 * database arguments are collected in `config->db_file_paths`
 */
void init_config(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	// Fill out with zeroes
	memset(config,0,sizeof(Config));

	// Application start time for total runtime reporting.
	config->app_start_time_ns = cur_time_monotonic_ns();

	// Absolute path name of the working directory
	// A directory where the program was executed
	config->running_dir = m_init(char,MEMORY_STRING);

	// Show progress bar
	config->progress = false;

	// Print remembered warnings and errors before exit
	config->show_remembered_messages_at_exit = false;

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

	// Show all compare output categories by default
	config->compare_filter = CF_NONE_SPECIFIED;

	// Managed array of root paths to traverse
	config->roots = m_init(memory);

	// The pointer to the primary database
	config->db = NULL;

	/// The flags parameter to sqlite3_open_v2()
	/// must include, at a minimum, one of the
	/// following flag combinations:
	///   - SQLITE_OPEN_READONLY
	///   - SQLITE_OPEN_READWRITE
	///   - SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
	/// Default value: RO
	config->sqlite_open_flag = SQLITE_OPEN_READONLY;

	// The path of DB file
	config->db_primary_file_path = m_init(char,MEMORY_STRING);

	// Set true only when the primary database path is ":memory:"
	config->db_primary_path_is_memory = false;

	// The name of DB file
	config->db_file_name = m_init(char,MEMORY_STRING);

	// Managed array of database file paths
	config->db_file_paths = m_init(memory);

	// Pointers to the array with database file names
	config->db_file_names = NULL;

	/// Allow or disallow database table
	/// initialization. True by default
	config->db_initialize_tables = true;

	/// Flag indicating whether the primary database file exists
	config->db_primary_file_exists = false;

	// The flag means that the DB already exists
	// and not empty
	config->db_contains_data = false;

	// Must be specified additionally in order
	// to remove from the database mention of
	// files that matches the regular expression
	// passed through the ignore option(s)
	// This is special protection against accidental
	// deletion of information from the database.
	config->db_drop_ignored = false;

	// Allow dropping database records for inaccessible files
	// (permission denied). Disabled by default.
	config->db_drop_inaccessible = false;

	/// Select database validation level: 'quick' for basic
	/// structure check, 'full' (default) for comprehensive
	/// integrity verification
	config->db_check_level = FULL;

	// Flag that reflects the presence of any changes
	// since the last research
	config->db_primary_file_modified = false;

	// Recursion depth limit. The depth of the traversal,
	// numbered from 0 to N, where a file could be found.
	// Representing the maximum of the starting
	// point (from root) of the traversal.
	// The root itself is numbered 0
	config->maxdepth = -1;

	// Ignore those relative paths
	// The string array of PCRE2 regular expressions
	config->ignore = NULL;

	// Suppress per-file log output for paths matched by --ignore
	config->quiet_ignored = false;

	// Include those relative paths even if
	// they were excluded via the --ignore option
	// The string array of PCRE2 regular expressions
	config->include = NULL;
	config->include_specified = false;

	// Relative paths whose checksums must never be recalculated
	// after the initial write. PCRE2 regular expressions.
	config->lock_checksum = NULL;

	// Force a full rehash for checksum-locked entries
	config->rehash_locked = false;

	// Perform a trial run with no changes made
	config->dry_run = false;

	// Allow hashing in dry-run mode (--dry-run=with-checksums)
	config->dry_run_with_checksums = false;

	// Define the comparison string
	const char *compare_string = "true";

	// Retrieve the value of the "TESTING" environment variable,
	// Validate if the environment variable TESTING exists
	// and if it match to "true" display ONLY testing
	// messages for System Testing purposes.
	const char *env_var = getenv("TESTING");

	// Check if it exists and compare it to "true"
	if(env_var != NULL && strcasecmp(env_var,compare_string) == 0)
	{
		// Global variable
		rational_logger_mode = TESTING;
	} else {
		// Global variable, default value
		rational_logger_mode = REGULAR;
	}

	/// This option prevents directory traversal from descending into
	/// directories that have a different device number than the file
	/// from  which the descent began
	config->start_device_only = false;

	// Pre-compiled PCRE2 patterns for --ignore, populated by compile_patterns() only if --ignore was specified
	config->ignore_pcre_compiled = NULL;

	// Pre-compiled PCRE2 patterns for --include, populated by compile_patterns() only if --include was specified
	config->include_pcre_compiled = NULL;

	// Pre-compiled PCRE2 patterns for --lock-checksum, populated by compile_patterns() only if --lock-checksum was specified
	config->lock_checksum_pcre_compiled = NULL;

	/// Track both file metadata (created/modified dates) and size changes
	/// for change detection. Out of the box, only size changes trigger
	/// a rescan. When enabled, any update to timestamps or file size
	/// will force a rescan and update the checksum in the database.
	config->watch_timestamps = false;

#if 0
	if(NULL != setlocale(LC_ALL,""))
	{
		slog(TRACE,"Enable locale support\n");
	} else {
		slog(TRACE,"Failed to set locale\n");
	}
#endif

	slog(TRACE,"Configuration initialization is finished\n");
}
