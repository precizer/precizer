#include "sute.h"

/**
 * @brief Run mixed-mode and boundary scenarios for libmem
 *
 * @return Return describing success or failure
 */
Return test_libmem_mode_boundaries(void)
{
	INITTEST;
	bool first_header = true;

	HEADER("Descriptor Boundaries");
	TEST(test_libmem_0063,"libmem string concat and raw byte concat operations…");
	TEST(test_libmem_0064,"libmem bounded raw and bounded string concat helpers…");

	HEADER("Mode Rejections");
	TEST(test_libmem_0065,"libmem mode mismatch negative cases are rejected without noisy output…");

	HEADER("Aliasing");
	TEST(test_libmem_0066,"libmem aliasing scenarios for string and data descriptors…");

	RETURN_STATUS;
}
