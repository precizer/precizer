#include "precizer.h"

/**
 *
 * Print out exit status
 *
*/
int exit_status(
	const Config *config,
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
		slog(config,false,"The %s has been interrupted smoothly. All data remain in integrity condition.\n",application_file_name);
		return(EXIT_SUCCESS);
	} else {
		if(status == SUCCESS){
			slog(config,false,"The %s completed its execution without any issues.\n",application_file_name);
			slog(config,false,"Enjoy your life!\n");
			return(EXIT_SUCCESS);
		}else{
			slog(config,false,"The %s has ended unexpectedly due to an error\n",application_file_name);
			return(EXIT_FAILURE);
		}
	}
}
