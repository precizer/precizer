#include "test_libtestitall_all.h"

/**
 * @brief Check stderr suppression and accepted stderr output
 *
 * @return Return status code
 */
Return test_libtestitall_0005(void)
{
	INITTEST;
	const char *stderr_command = "echo Example message to STDERR that should be suppressed 1>&2";

	// Suppress expected messages written to stderr
	ASSERT(SUCCESS == external_call(stderr_command,NULL,NULL,COMPLETED,STDERR_SUPPRESS));

	// Allow expected messages written to stderr
	ASSERT(SUCCESS == external_call(stderr_command,NULL,NULL,COMPLETED,STDERR_ALLOW));

	RETURN_STATUS;
}
