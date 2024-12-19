/**
 *
 * @file precizer.c
 * @brief Main file
 *
 */
#include "precizer.h"
#include <stdatomic.h>

/**
 * Global definitions
 *
 */

// Global variable controls signals to interrupt execution
// Atomic variable is very fast and will be called very often
_Atomic bool global_interrupt_flag = false;

// The global structure Config where all runtime settings will be stored
Config _config;
Config *config = &_config;

#ifndef TESTITALL
/**
 * @mainpage
 * @brief precizer is a CLI application designed to verify the integrity of files after synchronization.
 * The program recursively traverses directories and creates a database
 * of files and their checksums, followed by a quick comparison.
 *
 * @author Dennis Razumovsky
 * @details precizer specializes in managing vast file systems.
 * The program identifies synchronization errors by cross-referencing
 * data and checksums from various sources. Alternatively, it can be
 * used to crawl historical changes by comparing databases from the
 * same sources over different times.
 *
 */
int main(
	int  argc,
	char **argv
){

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Initialize configuration with values
	init_config();

	// Fill Config structure
	// parsing command line arguments
	run(parse_arguments(argc,argv));

	// Check all paths passed as arguments.
	// Are they directories and do they exist?
	run(detect_paths());

	// Initialize signals interception like Ctrl+C
	run(init_signals());

	// Generate DB file name if
	// not passed as an argument
	run(db_determine_name());

	// Validate database file existence and update existence flag
	run(db_file_validate_existence());

	// Define the database operation mode
	run(db_determine_mode());

	// Initialize SQLite database
	run(db_init());

	// Compare databases
	run(db_compare());

	// The current directory where app being executed
	run(determine_running_dir());

	// Check whether the database already exists or not yet
	run(db_contains_data());

	// Database file integrity check
	run(db_test(config->db_file_path));

	// Verify that the provided path arguments match
	// the paths stored in the database
	run(db_validate_paths());

	// Save new prefixes that has been passed as
	// arguments
	run(db_save_prefixes());

	// Verify that the provided paths exist and
	// are directories
	run(detect_paths());

	// Just get a statistic
	run(file_list(true));

	// Get file list and their CRC
	run(file_list(false));

	// Update the database. Remove files that
	// no longer exist.
	run(db_delete_missing_metadata());

	// Optimizing the space occupied by a database file.
	run(db_vacuum());

	// Print out whether there have been changes to
	// the file system and accordingly against the database
	// since the last research
	run(status_of_changes());

	// Free allocated memory
	// for arrays and variables
	free_config();

	return(exit_status(status,argv));
}
#endif // TESTITALL
