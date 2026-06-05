#include "sute.h"

/**
 * @brief Print memory telemetry collected during the libsha512 test run
 *
 * @return SUCCESS after the telemetry section is printed
 */
Return finish(void)
{
	Return status = SUCCESS;
	/* HEADER emits a leading newline only when first_header is false */
	bool first_header = false;

	HEADER("Telemetry");
	printf(WHITE);
	telemetry_summary();
	printf(RESET);

	return(SUCCESS);
}
