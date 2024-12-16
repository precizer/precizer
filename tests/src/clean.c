#include "sute.h"

Return clean(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	// Clear up all temporary files
	status = external_call("rm -rf ${TMPDIR};",0);

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	return(status);
}
