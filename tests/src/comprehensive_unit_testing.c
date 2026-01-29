#include "sute.h"

Return comprehensive_unit_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = run_external;

	run_external = INTERNAL_TEST;

	#include "comprehensive_unit_and_system_testing.cc"

	/* Mock only tecting in unit-mode so the linker wraps are active */

	#ifndef EVIL_EMPIRE_OS
	TEST(test0031,"Read error handling during hashing…");
	#endif

	run_external = prev_run_mode;

	RETURN_STATUS;
}
