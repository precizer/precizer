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

/**
 * @mainpage
 * @brief precizer is a CLI application designed to traverse directories recursively and build a database of files and their checksums.
 * @author Dennis Razumovsky
 *
 * @details The program is focused on the traversing of file systems
 * of gigantic size with subsequent quick comparison.
 * Using the program it is possible to find synchronization errors
 * by comparing data with files and their checksums from different
 * sources. Or it can be used to explore historical changes by
 * comparing databases from the same sources over different times.
 *
 */
int main(int argc,char **argv){

	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Initialize a structure where configuration will be putted
	Config *config = NULL;

	// Initialize configuration with values
	status = init_config(&config);

	if(SUCCESS == status)
	{
		// Fill Config structure
		// parsing command line arguments
		status = parse_arguments(config,argc,argv);
	}

	if(SUCCESS == status)
	{
		// Initialize signals interseption like Ctrl+C
		status = init_signals(config);
	}

	if(SUCCESS == status)
	{
		// Generate DB file name if not passed as an argument
		status = db_file_name(config);
	}

	if(SUCCESS == status)
	{
		// Initialize SQLite database
		status = db_init(config);
	}

	if(SUCCESS == status)
	{
		// Compare databases
		status = db_compare(config);
	}

	if(SUCCESS == status)
	{
		// The current directory where app being executed
		status = determine_running_dir(config);
	}

	if(SUCCESS == status)
	{
		// Check whether the database already exists or not yet
		status = db_already_exists(config);
	}

	if(SUCCESS == status)
	{
		// Check up if the paths that passed as arguments
		// exactly the same as saved against the database
		status = db_check_up_paths(config);
	}

	if(SUCCESS == status)
	{
		// Save new paths that has been passed as
		// arguments
		status = db_save_paths_into(config);
	}

	if(SUCCESS == status)
	{
		// Just get a statistic
		status = file_list(config,true);
	}

	if(SUCCESS == status)
	{
		// Get file list and their CRC
		status = file_list(config,false);
	}

	if(SUCCESS == status)
	{
		// Update the database. Remove files that
		// no longer exist.
		status = db_delete_missing_files_from(config);
	}

	if(SUCCESS == status)
	{
		// Print out whether there have been changes to
		// the file system and accordingly against the database
		// since the last research
		changes_status(config);
	}

	if(SUCCESS == status)
	{
		// Optimizing the space occupied by a database file.
		status = db_vacuum(config);
	}

	free_config(config);

	return(exit_status(config,status,argv));
}
