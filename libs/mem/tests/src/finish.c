#include "test_libmem_all.h"

Return finish(void)
{
	Return status = SUCCESS;
	/* HEADER emits a leading \n only when first_header is false */
	bool first_header = false;

	HEADER("Telemetry");
	printf(WHITE);
	telemetry_summary();
	printf(RESET);

	return(SUCCESS);
}
