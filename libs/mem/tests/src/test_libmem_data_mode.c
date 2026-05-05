#include "sute.h"

/**
 * @brief Run m_copy_buffer and pointer-reset example scenarios for libmem
 *
 * @return Return describing success or failure
 */
Return test_libmem_data_mode(void)
{
	INITTEST;
	bool first_header = true;

	HEADER("Low-Level Helpers");
	TEST(test_libmem_0052,"m_copy_buffer handles strings, structs, and clearing…");
	TEST(test_libmem_0053,"reset clears manually managed raw pointers…");
	TEST(test_libmem_0054,"guarded arithmetic helpers catch invalid size math…");

	HEADER("Data Invariants");
	TEST(test_libmem_0055,"Data-facing helpers reject descriptors with NULL data and non-zero length…");
	TEST(test_libmem_0056,"Data-mode resize keeps string metadata cleared…");
	TEST(test_libmem_0057,"Data-mode resize rejects stale string metadata…");
	TEST(test_libmem_0058,"Data-mode resize rejects reserves smaller than the logical payload…");

	HEADER("Data Transfers");
	TEST(test_libmem_0059,"libmem data wrappers support bytewise append and copy…");
	TEST(test_libmem_0060,"libmem data copy wrapper rejects partial destination tails…");
	TEST(test_libmem_0061,"libmem data wrappers support cross-type append and copy…");
	TEST(test_libmem_0062,"libmem data wrappers reject incompatible cross-type payloads…");

	RETURN_STATUS;
}
