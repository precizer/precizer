#include "precizer.h"

/**
 *
 * Interrupt loops smoothly
 * Interrupt when Ctrl+C (SIGTERM) or
 * kill -15 (SIGINT)
 *
 */
void signal_notify_quit_handler(int sig)
{
	atomic_store(&global_interrupt_flag,true);

	atomic_store(&global_return_status,HALTED);

	slog(EVERY,"Notify quit!\n");

	slog(EVERY,"The global return status and exit flag has been set to %s\n",show_status(global_return_status));

	if(sig==SIGTERM)
	{
		slog(EVERY,"Terminating the application. Please wait while the database will be closed smoothly…\n");
	}

	if(sig==SIGINT)
	{
		slog(EVERY,"Interrupting the application. Please wait while the database will be closed smoothly…\n");
	}

	/// Enable key echo in terminal
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag |= (ICANON|ECHO);
	tcsetattr(fileno(stdin),0,&term);
}
