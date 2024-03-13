/**
 *
 * @file precizer.c
 * @brief Main header file
 *
 */
#include "precizer.h"

/**
 * Global definition
 *
 */

// Global variable controls signals to interrupt execution
// Atomic variable is very fast and will be called very often
_Atomic bool global_interrupt_flag = false;

// The global structure Config where all runtime settings will be stored
Config _config;
Config *config = &_config;

/**
 * @mainpage
 * @brief precizer is a CLI application designed to check the integrity of files after synchronization. The program recursively traverses directories and creates a database of files and their checksums, followed by a quick comparison.
 *
 * @author Dennis Razumovsky
 * @details precizer is focused on work with gigantic file systems.
 * With the program it is possible to find synchronization errors
 * by comparing data with files and their checksums from different sources.
 * Or it can be used to crawling historical changes by comparing databases
 * from the same sources over different times.
 *
 */
int main(int argc,char **argv){

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Initialize configuration with values
	init_config();

	// Fill Config structure
	// parsing command line arguments
	status = parse_arguments(argc,argv);

	if(SUCCESS == status)
	{
		// Check all paths passed as arguments.
		// Are they directories and do they exist?
		status = detect_paths();
	}

	if(SUCCESS == status)
	{
		// Initialize signals interseption like Ctrl+C
		status = init_signals();
	}

	if(SUCCESS == status)
	{
		// Generate DB file name if not passed as an argument
		status = db_file_name();
	}

	if(SUCCESS == status)
	{
		// Initialize SQLite database
		status = db_init();
	}

	if(SUCCESS == status)
	{
		// Compare databases
		status = db_compare();
	}

	if(SUCCESS == status)
	{
		// The current directory where app being executed
		status = determine_running_dir();
	}

	if(SUCCESS == status)
	{
		// Check whether the database already exists or not yet
		status = db_already_exists();
	}

	if(SUCCESS == status)
	{
		// Check up the integrity of database file
		status = db_test(config->db_file_name);
	}

	if(SUCCESS == status)
	{
		// Check up if the paths that passed as arguments
		// exactly the same as saved against the database
		status = db_check_up_paths();
	}

	if(SUCCESS == status)
	{
		// Save new paths that has been passed as
		// arguments
		status = db_save_paths_into();
	}

	if(SUCCESS == status)
	{
		// Just get a statistic
		status = file_list(true);
	}

	if(SUCCESS == status)
	{
		// Get file list and their CRC
		status = file_list(false);
	}

	if(SUCCESS == status)
	{
		// Update the database. Remove files that
		// no longer exist.
		status = db_delete_missing_files_from();
	}

	if(SUCCESS == status)
	{
		// Optimizing the space occupied by a database file.
		status = db_vacuum();
	}

	if(SUCCESS == status)
	{
		// Print out whether there have been changes to
		// the file system and accordingly against the database
		// since the last research
		status_of_changes();
	}

	free_config();

	return(exit_status(status,argv));
}
