#include "precizer.h"

/**
 *
 * The structure Config where all runtime settings will be stored.
 * Initialization the structure elements by zerro.
 *
 */
void init_config(void)
{

	// Fill out with zerroes
	memset(config,0,sizeof(Config));

	// Max available size of a path
	config->running_dir_size = 0;

	// The pointer to the main database
	config->db = NULL;

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

	// The path of DB file
	config->db_file_path = NULL;

	// The name of DB file
	config->db_file_name = NULL;

	// The flag means that the DB already exists
	config->db_already_exists = false;

	// Flag that reflects the presence of any changes
	// since the last research
	config->something_has_been_changed = false;

	// Pointers to the array with database paths
	config->db_file_paths = NULL;

	// Pointers to the array with database file names
	config->db_file_names = NULL;

	// Don't produce any output
	config->silent = false;

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

	// Must be specified additionally in order
	// to remove from the database mention of
	// files that matches the regular expression
	// passed through the ignore option(s)
	// This is special protection against accidental
	// deletion of information from the database.
	config->db_clean_ignored = false;

	// Perform a trial run with no changes made
	config->dry_run = false;

}
