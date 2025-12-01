#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0006(void)
{
	INITTEST;

	ASSERT(SUCCESS == external_call("echo -n",GRACEFUL,false,false));

	RETURN_STATUS;
}
