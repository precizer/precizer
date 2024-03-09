#include "precizer.h"
#include <termios.h>

/**
 *
 * Clean allocated memory
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

	free(config->db_file_name);
}
