#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0006(void)
{
	INITTEST;

	ASSERT(SUCCESS == external_call("echo -n",COMPLETED,ALLOW_BOTH));

	RETURN_STATUS;
}
