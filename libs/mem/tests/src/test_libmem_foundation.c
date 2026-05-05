#include "sute.h"

/**
 * @brief Run foundation scenarios for descriptor allocation and telemetry
 *
 * @return Return describing success or failure
 */
Return test_libmem_foundation(void)
{
	INITTEST;
	bool first_header = true;

	HEADER("Allocation Lifecycle");
	TEST(test_libmem_0001,"Copy an array of 0 size…");
	TEST(test_libmem_0002,"libmem Memory allocator test 1…");
	TEST(test_libmem_0003,"libmem Memory allocator test 2…");
	TEST(test_libmem_0004,"libmem Memory allocator tests 4,5,6…");

	HEADER("Repeated Type Coverage");
	TEST(test_libmem_0005,"libmem generate multiple tests unsigned long long int type…");
	TEST(test_libmem_0006,"libmem generate multiple tests char type…");
	TEST(test_libmem_0007,"libmem generate multiple tests int type…");
	TEST(test_libmem_0008,"libmem generate multiple tests unsigned char type…");

	HEADER("Telemetry");
	TEST(test_libmem_0009,"libmem telemetry counters…");

	RETURN_STATUS;
}
