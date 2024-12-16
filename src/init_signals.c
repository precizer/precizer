#include "precizer.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

/**
 *
 * Initialize signals interception like Ctrl+C
 * The application controls signals like Ctrl+C to
 * prevent database corruption.
 * It always try to complete work in correct way and
 * sync data from memory to disk even user interrupts
 * running of the program.
 *
 */
Return init_signals(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	/// Disable key echo in terminal
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag &= (unsigned int)~(ICANON|ECHO); // knock down keybuffer
	tcsetattr(fileno(stdin),TCSANOW,&term);

	// kill -USR2 <pid>
	if((signal(SIGUSR2,&notify_quit_handler)==SIG_ERR)!=0)
	{
		status = FAILURE;
		slog(ERROR,"Failed set signal SIGUSR2\n");
	} else {
		slog(TRACE,"Set signal SIGUSR2 OK:pid:%i\n",getpid());
	}

	// Ctrl-C
	if((signal(SIGINT,&notify_quit_handler)==SIG_ERR)!=0)
	{
		status = FAILURE;
		slog(ERROR,"Failed set signal SIGINT\n");
	} else {
		slog(TRACE,"Set signal SIGINT OK:pid:%i\n",getpid());
	}

	// Default kill Termination signal (15)
	if((signal(SIGTERM,&notify_quit_handler)==SIG_ERR)!=0)
	{
		status = FAILURE;
		slog(ERROR,"Failed set signal SIGTERM\n");
	} else {
		slog(TRACE,"Set signal SIGTERM OK:pid:%i\n",getpid());
	}

	slog(TRACE,"Signals initialized\n");

	return status;
}
