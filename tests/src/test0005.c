#include "sute.h"

/**
 *
 * Another simple test case
 *
 */
Return test0005(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Example of suppress messages to STDERR
	ASSERT(SUCCESS == external_call("echo Example message to STDERR that should be suppressed 1>&2",0,true,false));

	RETURN_STATUS;
}
