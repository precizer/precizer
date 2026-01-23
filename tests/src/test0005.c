#include "sute.h"

/**
 *
 * Another simple test case
 *
 */
Return test0005(void)
{
	INITTEST;

	// Example of suppress messages to STDERR
	const char *command = "echo Example message to STDERR that should be suppressed 1>&2";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,STDERR_SUPPRESS));

	// Example of NOT suppressed messages to STDERR
	command = "echo Example message to STDERR that should be suppressed 1>&2";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,STDERR_ALLOW));

	RETURN_STATUS;
}
