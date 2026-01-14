#include "sute.h"

Return clean(void)
{
	INITTEST;

	// Clear up all temporary files
	status = external_call("chmod -R a+rwX ${TMPDIR}; rm -rf ${TMPDIR};",COMPLETED,ALLOW_BOTH);

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	return(status);
}
