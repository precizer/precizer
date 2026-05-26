#include "sute.h"

/**
 * @brief Check external command success and failure expectations
 *
 * @return Return status code
 */
Return test_libtestitall_0004(void)
{
	INITTEST;

	const char *command = "true > /dev/null";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,STDERR_SUPPRESS));

	command = "false";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,FAILURE,STDOUT_SUPPRESS));

	command = "true";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,STDERR_SUPPRESS));

	RETURN_STATUS;
}
