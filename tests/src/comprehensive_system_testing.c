#include "sute.h"

Return comprehensive_system_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = run_external;

	run_external = EXTERNAL_CALL;

	#include "comprehensive_unit_and_system_testing.cc"

	run_external = prev_run_mode;

	RETURN_STATUS;
}
