#include "test_libmem_utils.h"

/**
 * @brief Random-size descriptor round-trip for unsigned long long int descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_1(void)
{
	INITTEST;

	#define TYPE unsigned long long int
	#include "test_libmem_0005.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size descriptor round-trip for char descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_2(void)
{
	INITTEST;

	#define TYPE char
	#include "test_libmem_0005.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size descriptor round-trip for int descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_3(void)
{
	INITTEST;

	#define TYPE int
	#include "test_libmem_0005.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size descriptor round-trip for unsigned char descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_4(void)
{
	INITTEST;

	#define TYPE unsigned char
	#include "test_libmem_0005.cc"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Run random-size descriptor round-trip coverage across repeated element types
 *
 * The suite verifies unsigned long long int, char, int, and unsigned char
 * descriptors with the same SHA-512-backed round-trip template. Each subtest
 * allocates a heap buffer of random size, hashes it, copies it into a descriptor
 * of the matching type, hashes the descriptor contents, and compares the two
 * digests
 *
 * @return Return describing success or failure
 */
Return test_libmem_0005(void)
{
	INITTEST;

	SLOWTEST;

	TEST(test_libmem_0005_1,"Random-size unsigned long long int descriptor round-trip…");
	TEST(test_libmem_0005_2,"Random-size char descriptor round-trip…");
	TEST(test_libmem_0005_3,"Random-size int descriptor round-trip…");
	TEST(test_libmem_0005_4,"Random-size unsigned char descriptor round-trip…");

	RETURN_STATUS;
}
