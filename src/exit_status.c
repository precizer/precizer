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
		slog(false,"The %s has been interrupted smoothly. All data remain in integrity condition.\n",application_file_name);
		return(EXIT_SUCCESS);
	} else {
		if(status == SUCCESS){
			slog(false,"The %s completed its execution without any issues.\n",application_file_name);
			slog(false,"Enjoy your life!\n");
			return(EXIT_SUCCESS);
		}else{
			slog(false,"The %s unexpectedly ended due to an error.\n",application_file_name);
			return(EXIT_FAILURE);
		}
	}
}
