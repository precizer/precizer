#include "sute.h"

Return clean(void)
{
	INITTEST;

	// Clear up all temporary files
	const char *command = "chmod -R a+rwX ${TMPDIR};"
	        "rm -rf ${TMPDIR};";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH);

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	return(status);
}
