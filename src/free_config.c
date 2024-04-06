#include "precizer.h"
#include <termios.h>
#include <unistd.h>

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
	tcgetattr(fileno(stdin), &term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin), 0, &term);

	/* Close previously used DB */
	sqlite3_close(config->db);

	free(config->running_dir);

	free(config->db_file_path);

	free(config->db_file_name);

	// Free memory of string array
	if(config->db_file_names != NULL)
	{
		for (int i = 0; config->db_file_names[i] != NULL; ++i) {
			free(config->db_file_names[i]);
		}
		free(config->db_file_names);
	}

	// Free memory of string array
	if(config->ignore != NULL)
	{
		for (int i = 0; config->ignore[i] != NULL; ++i) {
			free(config->ignore[i]);
		}
		free(config->ignore);
	}

	// Free memory of string array
	if(config->include != NULL)
	{
		for (int i = 0; config->include[i] != NULL; ++i) {
			free(config->include[i]);
		}
		free(config->include);
	}

	// DB File sync to disk
	sync();
}
