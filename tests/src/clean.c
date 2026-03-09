#include "sute.h"

Return clean(void)
{
	INITTEST;

	// Clear up all temporary files
	const char *command = "chmod -R a+rwX ${TMPDIR};";

	ASSERT(SUCCESS == external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	// Empty relative path resolves to TMPDIR itself, so this removes the whole temporary test root
	ASSERT(SUCCESS == delete_path(""));

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	return(status);
}
