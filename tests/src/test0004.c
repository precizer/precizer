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

	ASSERT(SUCCESS == external_call("echo -n > /dev/null",SUCCESS,false,false));
	ASSERT(SUCCESS == external_call("false",FAILURE,false,false));
	ASSERT(SUCCESS == external_call("true",SUCCESS,false,false));

	RETURN_STATUS;
}
