#include "sute.h"

/* Mock only tecting in unit-mode so the linker wraps are active */
Return comprehensive_mock_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = run_external;

	run_external = INTERNAL_TEST;

	TEST(test0031,"Read error handling during hashing…");

	run_external = prev_run_mode;

	RETURN_STATUS;
}
