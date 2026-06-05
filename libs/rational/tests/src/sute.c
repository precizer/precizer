#include "sute.h"

/**
 * @brief Run the top-level test suite executable for librational
 *
 * @return Process exit status produced by the test runner macros
 */
int main(void)
{
	SUTESTART;

	HEADER("Testing of librational");
	TEST(test_librational_all,"librational test set");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
