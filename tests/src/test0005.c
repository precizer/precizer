#include "sute.h"

/**
 *
 * Another simple test case
 *
 */
Return test0005(void)
{
	INITTEST;

	// Example of suppress messages to STDERR
	ASSERT(SUCCESS == external_call("echo Example message to STDERR that should be suppressed 1>&2",0,true,false));

	RETURN_STATUS;
}
