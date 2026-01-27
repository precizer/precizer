#include "sute.h"

Return comprehensive_unit_testing(void)
{
	INITTEST;

	run_external = INTERNAL_TEST;

	#include "comprehensive_unit_and_system_testing.cc"

	/* Mock only tecting in unit-mode so the linker wraps are active */

	TEST(test0031,"Read error handling during hashing…");

	RETURN_STATUS;
}
