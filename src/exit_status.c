#include "precizer.h"

/**
 *
 * Print out an exit status
 *
 */
int exit_status(
	Return       status,
	char * const *argv)
{
	/*
	 *
	 * Exit
	 *
	 */

	const char *application_file_name = basename(argv[0]);

	if(global_interrupt_flag == true)
	{
		slog(EVERY,"The %s has been interrupted smoothly. All data remain in integrity condition\n",application_file_name);
		slog(EVERY,"Exit status » %s\n",show_status(status));
		return((int)status);
	} else {
		if(TRIUMPH & status)
		{
			if((INFO & status) == 0)
			{
				slog(EVERY,"The %s completed as expected\n",application_file_name);
			}
			slog(EVERY,"Exit status » %s\n",show_status(status));
			slog(REGULAR,"Enjoy your life!\n");
			return((int)COMPLETED);
		} else {
			slog(ERROR,"The %s process terminated unexpectedly due to an error\n",application_file_name);
			slog(ERROR,"Exit status » %s\n",show_status(status));
			return((int)status);
		}
	}
}
