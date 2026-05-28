#include "test_libmem_all.h"
#include "sha512.h"

/**
 * @brief Random-size bounded-buffer import for unsigned long long int descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_1(void)
{
	INITTEST;

	#define TYPE unsigned long long int
	#include "test_libmem_0005.tpl"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size bounded-buffer import for char descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_2(void)
{
	INITTEST;

	#define TYPE char
	#include "test_libmem_0005.tpl"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size bounded-buffer import for int descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_3(void)
{
	INITTEST;

	#define TYPE int
	#include "test_libmem_0005.tpl"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Random-size bounded-buffer import for unsigned char descriptors
 *
 * @return Return describing success or failure
 */
static Return test_libmem_0005_4(void)
{
	INITTEST;

	#define TYPE unsigned char
	#include "test_libmem_0005.tpl"
	#undef TYPE

	RETURN_STATUS;
}

/**
 * @brief Run random-size bounded-buffer import coverage across repeated element types
 *
 * The suite verifies unsigned long long int, char, int, and unsigned char
 * descriptors with the same SHA-512-backed import template. Each subtest uses
 * a fixed-capacity stack source buffer, selects random payload byte ranges,
 * imports each range through m_copy_buffer(...), hashes the resulting
 * descriptor view, and compares it with the original source digest
 *
 * @return Return describing success or failure
 */
Return test_libmem_0005(void)
{
	INITTEST;

	SLOWTEST;

	TEST(test_libmem_0005_1,"Random-size unsigned long long int bounded-buffer import");
	TEST(test_libmem_0005_2,"Random-size char bounded-buffer import");
	TEST(test_libmem_0005_3,"Random-size int bounded-buffer import");
	TEST(test_libmem_0005_4,"Random-size unsigned char bounded-buffer import");

	RETURN_STATUS;
}
