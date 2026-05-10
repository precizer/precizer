#include "test_libmem_utils.h"

/**
 * @brief Random-size descriptor round-trip across element types — SHA-512 verified
 *
 * For each of unsigned long long int, char, int, and unsigned char runs the shared template that
 * allocates a heap buffer of random size, hashes it, copies it into a descriptor of the matching
 * type, hashes the descriptor's contents, and asserts the two SHA-512 digests match. The template
 * iterates CYCLES x CYCLES times per type to vary sizes through resize and realloc paths
 *
 * @return Return describing success or failure
 */
Return test_libmem_0005(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE unsigned long long int
	#include "test_libmem_0005.cc"
	#undef TYPE

	#define TYPE char
	#include "test_libmem_0005.cc"
	#undef TYPE

	#define TYPE int
	#include "test_libmem_0005.cc"
	#undef TYPE

	#define TYPE unsigned char
	#include "test_libmem_0005.cc"
	#undef TYPE

	RETURN_STATUS;
}
