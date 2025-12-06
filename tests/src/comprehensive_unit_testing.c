#include "sute.h"

Return comprehensive_unit_testing(void)
{
	INITTEST;

	run_external = INTERNAL_TEST;

	#include "comprehensive_unit_and_system_testing.cc"

	RETURN_STATUS;
}
