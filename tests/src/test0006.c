#include "sute.h"

/**
 *
 * Example test
 *
 */
Return test0006(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	ASSERT(SUCCESS == external_call("echo -n",0));

	RETURN_STATUS;
}
