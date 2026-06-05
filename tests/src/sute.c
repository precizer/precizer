#include "sute.h"

/**
 * @brief Run the top-level test suite executable
 *
 * @return Process exit status produced by the test runner macros
 */
int main(void)
{
	SUTESTART;

	HEADER("Preparations");
	TEST(prepare,"Preparation for tests");

	HEADER("Testing of built-in libraries");
	SUTE(bundled_libraries,"Bundled libraries test set");

	HEADER("Unit Testing of precizer");
	SUTE(function_unit_testing,"Function-level unit tests");
	TEST(comprehensive_unit_testing,"Comprehensive Unit testing");

	HEADER("System Testing of precizer");
	TEST(comprehensive_system_testing,"Comprehensive System testing");

	/* Mock only tecting in unit-mode so the linker wraps are active */
	#ifndef EVIL_EMPIRE_OS
	HEADER("Mock-Based Testing of precizer");
	TEST(comprehensive_mock_testing,"Comprehensive Mock testing");
	#endif

	HEADER("Clean results");
	RUN(clean,"Temporary data cleanup");

	RUN(finish,"Telemetry");

	HEADER("Finishing");
	SUTEDONE;
}
