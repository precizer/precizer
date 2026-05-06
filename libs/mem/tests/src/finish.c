#include "sute.h"

Return finish(void)
{
	Return status = SUCCESS;
	/* HEADER emits a leading \n only when first_header is false */
	bool first_header = false;

	HEADER("Telemetry");
	printf(WHITE);
	telemetry_show();
	printf(RESET);

	return(SUCCESS);
}
