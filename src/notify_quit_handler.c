#include "precizer.h"

/**
 *
 * Interrupt loops smoothly
 * Interrupt when Ctrl+C (SIGTERM) or
 * kill -15 (SIGINT)
 *
 */
void notify_quit_handler(int sig)
{
        /*
         * Only async-signal-safe operations are allowed inside
         * the signal handler.  Using printf() or other stdio
         * functions may lead to undefined behaviour and could
         * interrupt the graceful shutdown procedure.
         */

        atomic_store(&global_interrupt_flag,true);
        atomic_store(&global_return_status,HALTED);

        const char msg_term[] =
                "Terminating the application. Please wait while the database will be closed smoothly…\n";
        const char msg_int[] =
                "Interrupting the application. Please wait while the database will be closed smoothly…\n";

        if(sig==SIGTERM)
        {
                (void)write(STDOUT_FILENO,msg_term,sizeof(msg_term)-1);
        }

        if(sig==SIGINT)
        {
                (void)write(STDOUT_FILENO,msg_int,sizeof(msg_int)-1);
        }
}
