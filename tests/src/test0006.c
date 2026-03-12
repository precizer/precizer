#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0006(void)
{
	INITTEST;

	const char *command = "printf '%s' ''";
	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
