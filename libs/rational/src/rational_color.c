#include "rational.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

_Atomic RATIONAL_COLOR_MODE rational_color_mode = COLOR_MODE_AUTO;

/**
 * @brief Determine whether terminal colors may be written to a stream
 *
 * @details Always mode permits styling for any non-NULL stream, while never
 *          mode disables it. In automatic mode, the presence of NO_COLOR or
 *          TERM=dumb disables styling. Otherwise, the decision uses the
 *          supplied stream's terminal connection at the time of the call,
 *          including redirections made by a test runner. A terminal connection
 *          does not establish support for a particular color depth or control
 *          sequence. The function preserves errno and leaves buffering unchanged
 *
 * @param stream Output stream that would receive terminal color sequences;
 *               NULL disables styling in every mode
 * @return true when terminal colors may be written, otherwise false
 */
bool rational_color_is_enabled(FILE *stream)
{
	/* Keep the caller's error code so terminal-detection calls cannot change
	   the error reported by code that runs after this policy check */
	const int saved_errno = errno;

	/* Read the shared mode atomically once and use that snapshot throughout
	   this call. Relaxed ordering reads the setting without synchronizing
	   access to unrelated data */
	const RATIONAL_COLOR_MODE mode = atomic_load_explicit(&rational_color_mode,memory_order_relaxed);

	/* Start with styling disabled. Never mode and a missing output stream
	   leave this result unchanged because neither enabling branch applies */
	bool color_is_enabled = false;

	/* Always mode enables styling for the supplied stream without consulting
	   environment variables or checking whether the stream is a terminal */
	if(stream != NULL && COLOR_MODE_ALWAYS == mode)
	{
		color_is_enabled = true;
	} else if(stream != NULL && COLOR_MODE_AUTO == mode && getenv("NO_COLOR") == NULL){
		/* Automatic detection runs only when NO_COLOR is absent. Its presence
		   disables automatic styling even when its value is an empty string */
		const char *term = getenv("TERM");

		/* The exact TERM value dumb disables automatic styling. An unset TERM
		   or any other value allows the stream's terminal check to proceed */
		if(term == NULL || 0 != strcmp(term,"dumb"))
		{
			/* Resolve the descriptor of this specific stream on each check.
			   This detects its current redirection and lets stdout and stderr
			   receive independent decisions */
			const int stream_descriptor = fileno(stream);

			/* Require a valid descriptor connected to a terminal or
			   pseudoterminal. Files, pipes, and failed checks leave styling
			   disabled. This tests the connection, not terminal capabilities */
			if(stream_descriptor >= 0 && isatty(stream_descriptor) == 1)
			{
				color_is_enabled = true;
			}
		}
	}

	/* Restore the caller's error code and return the decision. The caller
	   uses it to select styled or plain text for this output stream */
	errno = saved_errno;
	return(color_is_enabled);
}
