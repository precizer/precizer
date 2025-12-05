#include "sute.h"

Return comprehensive_system_testing(void)
{
	INITTEST;

	run_external = EXTERNAL_CALL;

	#include "comprehensive_unit_and_system_testing.cc"

	RETURN_STATUS;
}
