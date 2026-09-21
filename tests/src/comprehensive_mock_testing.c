#include "sute.h"

/* Run mock-only tests in unit mode so linker wraps are active */
Return comprehensive_mock_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = testitall_runit_mode;

	testitall_runit_mode = INTERNAL_TEST;

	TEST(test0031,"Read error handling during hashing");

	testitall_runit_mode = prev_run_mode;

	RETURN_STATUS;
}
