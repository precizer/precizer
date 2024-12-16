#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0004(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	ASSERT(SUCCESS == external_call("echo -n > /dev/null",0));

	RETURN_STATUS;
}
