#include "sute.h"

/**
 *
 * Just simple example test
 *
 */
Return test0004(void)
{
	INITTEST;

	ASSERT(SUCCESS == external_call("echo -n > /dev/null",COMPLETED,ALLOW_BOTH));
	ASSERT(SUCCESS == external_call("false",FAILURE,ALLOW_BOTH));
	ASSERT(SUCCESS == external_call("true",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
