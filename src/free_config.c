#include "precizer.h"

/**
 * @brief Free global configuration resources and restore terminal state
 */
void free_config(void)
{
	// Restore terminal echo and canonical mode
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin),0,&term);

	free(config->running_dir);

	(void)del(conf(db_primary_file_path));

	(void)del(conf(db_file_name));

	// Free and reset string arrays stored in the global config
	free_string_array(&(config->db_file_names));

	// Free and reset string arrays stored in the global config
	free_string_array(&(config->ignore));

	// Free and reset string arrays stored in the global config
	free_string_array(&(config->include));

	// Free and reset string arrays stored in the global config
	free_string_array(&(config->lock_checksum));
}
