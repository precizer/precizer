#include "test_librational_all.h"

/**
 * @brief Run all public librational test suites
 *
 * @return Return status code
 */
Return test_librational_all(void)
{
	INITTEST;

	bool first_header = true;

	HEADER("Formatting");
	SUTE(test_librational_0001,"librational formatting helper test set");
	SUTE(test_librational_0002,"librational itoa conversion helpers");

	HEADER("Time");
	SUTE(test_librational_0003,"librational time helpers, duration formatter and ISO date helper");

	HEADER("Report and Logger");
	SUTE(test_librational_0004,"librational report(), serp() and slog() helpers");

	HEADER("Return Handling");
	SUTE(test_librational_0005,"librational Return flow, normalization and status text");

	RETURN_STATUS;
}
