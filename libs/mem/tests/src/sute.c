#include "sute.h"

/**
 * @brief Run the top-level test suite executable for libmem
 *
 * @return Process exit status produced by the test runner macros
 */
int main(void)
{
	SUTESTART;

	HEADER("Testing of libmem");
	TEST(test_libmem_all,"libmem test set…");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
