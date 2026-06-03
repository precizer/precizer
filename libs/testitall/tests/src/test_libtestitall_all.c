#include "test_libtestitall_all.h"

/**
 * @brief Run every libtestitall test group
 * @details Groups checks by the public framework behavior they exercise
 *
 * @return Return status for the complete libtestitall test group
 */
Return test_libtestitall_all(void)
{
	INITTEST;

	bool first_header = true;

	HEADER("Runner Examples");
	// Test function name and its short description
	TEST(test_libtestitall_0001,"External commands report expected return codes");
	TEST(test_libtestitall_0002,"External stderr follows the selected capture policy");
	TEST(test_libtestitall_0003,"Silent external commands complete successfully");

	HEADER("Output Capture");
	TEST(test_libtestitall_0004,"Captured stdout matches an in-source expectation");

	HEADER("Temporary Directories");
	TEST(test_libtestitall_0005,"Temporary directory roots and names are selected correctly");
	TEST(test_libtestitall_0006,"Path construction reports invalid input diagnostics");

	HEADER("String Helpers");
	TEST(test_libtestitall_0007,"Trailing EOL removal preserves libmem string invariants");
	TEST(test_libtestitall_0008,"Memory string comparison produces the expected diff");

	RETURN_STATUS;
}
