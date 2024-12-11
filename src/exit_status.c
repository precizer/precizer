#include "precizer.h"

/**
 *
 * Print out an exit status
 *
*/
int exit_status(
	Return status,
	char **argv
){
	/*
	 *
	 * Exit
	 *
	 */

	const char *application_file_name = basename(argv[0]);

	if(global_interrupt_flag == true)
	{
		slog(EVERY,"The %s has been interrupted smoothly. All data remain in integrity condition\n",application_file_name);
		return(EXIT_SUCCESS);
	} else {
		if(SUCCESS == status){
			slog(EVERY,"The %s completed its execution without any issues\n",application_file_name);
			slog(REGULAR,"Enjoy your life!\n");
			return(EXIT_SUCCESS);
		}else{
			slog(ERROR,"The %s process terminated unexpectedly due to an error\n",application_file_name);
			return(EXIT_FAILURE);
		}
	}
}
