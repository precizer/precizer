#include "sute.h"

/**
 * @brief Run the main application system test suite
 *
 * @details
 * System tests answer the question a user would ask after building the
 * program: does the real `precizer` executable behave correctly when it is
 * launched with command-line options, real files, real directories, and a real
 * SQLite database. They are broader than unit tests, which usually check one
 * function or one small rule in isolation. They are also different from
 * mock-based tests, which deliberately replace selected operating-system or
 * library behavior so rare error paths can be tested safely and repeatably.
 *
 * Some behavior can only be trusted through system tests because it depends on
 * process boundaries and external state. Examples include command-line parsing
 * as a user sees it, stdout and stderr output, filesystem traversal, database
 * persistence, background runs, interruption handling, and crash/resume
 * scenarios. Those checks need the built application, its test workspace, and
 * the operating system to work together instead of only calling internal C
 * functions directly
 *
 */
Return comprehensive_system_testing(void)
{
	INITTEST;

	enum run_mode prev_run_mode = run_external;

	run_external = EXTERNAL_CALL;

	#include "comprehensive_unit_and_system_testing.cc"

	SUTE(test0038,"SHA512 checkpoint and resume persistence scenarios");

	run_external = prev_run_mode;

	RETURN_STATUS;
}
