#include "sute.h"

Return clean(void)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	// Clear up all temporary files
	const char *command = "chmod -R a+rwX ${TMPDIR};";

	run(external_call(command,NULL,NULL,COMPLETED,ALLOW_BOTH));
	// Empty relative path resolves to TMPDIR itself, so this removes the whole temporary test root
	run(delete_path(""));

	if(SUCCESS == status)
	{
		echo(EXTEND,"finished");
	}

	deliver(status);
}
