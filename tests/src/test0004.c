#include "sute.h"

/**
 * @brief Simple example test with external calls
 */
Return test0004(void)
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
