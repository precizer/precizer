#include "sute.h"

Return comprehensive_unit_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = testitall_runit_mode;

	testitall_runit_mode = INTERNAL_TEST;

	#include "comprehensive_unit_and_system_testing.cc"

	testitall_runit_mode = prev_run_mode;

	RETURN_STATUS;
}
