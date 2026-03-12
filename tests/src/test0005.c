#include "sute.h"

/**
 *
 * Another simple test case
 *
 */
Return test0005(void)
{
	INITTEST;
	const char *stderr_command = "echo Example message to STDERR that should be suppressed 1>&2";

	// Example of suppress messages to STDERR
	ASSERT(SUCCESS == external_call(stderr_command,NULL,NULL,COMPLETED,STDERR_SUPPRESS));

	// Example of NOT suppressed messages to STDERR
	ASSERT(SUCCESS == external_call(stderr_command,NULL,NULL,COMPLETED,STDERR_ALLOW));

	RETURN_STATUS;
}
