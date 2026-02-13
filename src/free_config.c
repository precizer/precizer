#include "precizer.h"

/**
 *
 * Clean up allocated memory
 *
 */
void free_config(void)
{
	/// Enable key echo in terminal (return back
	/// default settings)
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin),0,&term);

	free(config->running_dir);

	free(config->db_primary_file_path);
	config->db_primary_file_path = NULL;

	free(config->db_file_name);
	config->db_file_name = NULL;

	// Free memory of string array
	free_string_array((config)->db_file_names);

	// Free memory of string array
	free_string_array((config)->ignore);

	// Free memory of string array
	free_string_array((config)->include);

	// Free memory of string array
	free_string_array((config)->lock_checksum);
}
