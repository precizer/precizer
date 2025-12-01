#include "sute.h"

/**
 *
 * Just simple example test
 *
 */
Return test0004(void)
{
	INITTEST;

	ASSERT(SUCCESS == external_call("echo -n > /dev/null",GRACEFUL,false,false));
	ASSERT(SUCCESS == external_call("false",FAILURE,false,false));
	ASSERT(SUCCESS == external_call("true",GRACEFUL,false,false));

	RETURN_STATUS;
}
