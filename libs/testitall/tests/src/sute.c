#include "sute.h"

/**
 * @brief Run the standalone libtestitall test program
 *
 * @return Process exit status produced by the test runner macros
 */
int main(void)
{
	SUTESTART;

	HEADER("Testing of libtestitall");
	TEST(test_libtestitall_all,"libtestitall framework and helper behavior checks...");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
