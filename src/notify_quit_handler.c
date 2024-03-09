#include "precizer.h"
#include <signal.h>
#include <termios.h>

/**
 * Interrupt the loop smoothly
 * Interrupt when Ctrl+C
 */
void notify_quit_handler
(
	int sig
){
	printf("Notify quit!\n");
	global_interrupt_flag = true;
	if (sig==SIGTERM){
		printf("Terminating the application. Please wait while the database will be closed smoothly...\n");
	}
	if (sig==SIGINT){
		printf("Interrupting the application. Please wait while the database will be closed smoothly...\n");
	}

	/// Enable key echo in terminal
	struct termios term;
	tcgetattr(fileno(stdin), &term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin), 0, &term);
}
