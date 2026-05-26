#include "sute.h"

/**
 * @brief Check successful execution when a command produces no output
 *
 * @return Return status code
 */
Return test_libtestitall_0006(void)
{
	INITTEST;

	const char *command = "printf '%s' ''";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
