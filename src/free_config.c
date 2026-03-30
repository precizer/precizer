#include "precizer.h"

/**
 * @brief Free global configuration resources and restore terminal state
 */
void free_config(void)
{
	/* This function was reviewed line by line by a human and is not AI-generated
	   Any change to this function requires separate explicit approval */

	// Restore terminal echo and canonical mode only when stdin is a real terminal
	if(isatty(fileno(stdin)))
	{
		struct termios term;
		tcgetattr(fileno(stdin),&term);
		term.c_lflag |= (ICANON|ECHO);
		tcsetattr(fileno(stdin),0,&term);
	}

	(void)del(conf(running_dir));

	(void)del(conf(db_primary_file_path));

	(void)del(conf(db_file_name));

	// Database file name list built by db_determine_name()
	free_string_array(&(config->db_file_names));

	// PCRE2 pattern strings supplied via --ignore
	free_string_array(&(config->ignore));

	// PCRE2 pattern strings supplied via --include
	free_string_array(&(config->include));

	// PCRE2 pattern strings supplied via --lock-checksum
	free_string_array(&(config->lock_checksum));

	// Pre-compiled PCRE2 patterns for --ignore
	free_compiled_array(&config->ignore_pcre_compiled);

	// Pre-compiled PCRE2 patterns for --include
	free_compiled_array(&config->include_pcre_compiled);

	// Pre-compiled PCRE2 patterns for --lock-checksum
	free_compiled_array(&config->lock_checksum_pcre_compiled);
}
