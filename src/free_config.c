#include "precizer.h"

/**
 * @brief Release resources owned by the global configuration
 *
 * The function restores terminal input mode when stdin is a terminal, then
 * releases every dynamically managed configuration field. Libmem descriptors
 * are deleted through the matching `m_del()` or `m_array_del()` helpers, while
 * plain pointer arrays and compiled PCRE2 arrays use their own free helpers
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

	// Runtime working directory used to resolve relative paths
	(void)m_del(conf(running_dir));

	// Primary database file path used for read and write operations
	(void)m_del(conf(db_primary_file_path));

	// Primary database file name used in diagnostics and derived path handling
	(void)m_del(conf(db_file_name));

	// Filesystem traversal roots supplied as positional arguments outside --compare
	(void)m_array_del(conf(roots));

	// Database file paths supplied as positional arguments in --compare mode
	(void)m_array_del(conf(db_file_paths));

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
