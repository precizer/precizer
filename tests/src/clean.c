#include "sute.h"

Return clean(void)
{
	INITTEST;

	// Clear up all temporary files
	status = external_call("rm -rf ${TMPDIR};",COMPLETED,false,false);

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	return(status);
}
