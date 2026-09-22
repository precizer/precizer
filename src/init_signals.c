#include "precizer.h"

/**
 * @brief Configure terminal input and handlers for graceful interruption
 *
 * Initialize signals interception like Ctrl+C
 * The application controls signals like Ctrl+C to
 * prevent database corruption.
 * It always try to complete work in correct way and
 * sync data from memory to disk even user interrupts
 * running of the program. MSYS console sessions disable QuickEdit
 * during processing so mouse selection does not pause file traversal
 *
 */
Return init_signals(void)
{
	/* Status returned by this function through provide()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	/// Disable key echo in terminal
	struct termios term;
	tcgetattr(fileno(stdin),&term);
	term.c_lflag &= (unsigned int)~(ICANON|ECHO);  // knock down keybuffer
	tcsetattr(fileno(stdin),TCSANOW,&term);

	// kill -USR2 <pid>
	if((signal(SIGUSR2,&signal_notify_quit_handler)==SIG_ERR)!=0)
	{
		slog(ERROR,"Failed set signal SIGUSR2\n");
		status = FAILURE;
	} else {
		slog(TRACE,"Set signal SIGUSR2 OK:pid:%i\n",getpid());
	}

	// Ctrl-C
	if((signal(SIGINT,&signal_notify_quit_handler)==SIG_ERR)!=0)
	{
		slog(ERROR,"Failed set signal SIGINT\n");
		status = FAILURE;
	} else {
		slog(TRACE,"Set signal SIGINT OK:pid:%i\n",getpid());
	}

	// Default kill Termination signal (15)
	if((signal(SIGTERM,&signal_notify_quit_handler)==SIG_ERR)!=0)
	{
		slog(ERROR,"Failed set signal SIGTERM\n");
		status = FAILURE;
	} else {
		slog(TRACE,"Set signal SIGTERM OK:pid:%i\n",getpid());
	}

#ifdef __MSYS__
	if(SUCCESS == status)
	{
		// Disable QuickEdit so mouse selection in the Windows console cannot pause file traversal
		disable_console_quick_edit();
	}
#endif

	slog(TRACE,"Signals initialized\n");

	provide(status);
}
