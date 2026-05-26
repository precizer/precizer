#include "sute.h"

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
	TEST(test_libtestitall_0004,"External commands report expected return codes...");
	TEST(test_libtestitall_0005,"External stderr follows the selected capture policy...");
	TEST(test_libtestitall_0006,"Silent external commands complete successfully...");

	HEADER("Output Capture");
	TEST(test_libtestitall_0010,"Captured stdout matches an in-source expectation...");

	HEADER("Temporary Directories");
	TEST(test_libtestitall_0035,"Temporary directory roots and names are selected correctly...");

	HEADER("String Helpers");
	TEST(test_libtestitall_0038,"Trailing EOL removal preserves libmem string invariants...");

	RETURN_STATUS;
}
