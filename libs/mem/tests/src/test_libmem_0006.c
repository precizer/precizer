#include "test_libmem_utils.h"

/**
 * @brief Multiple tests with char type and different array sizes
 *
 */
Return test_libmem_0006(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE char
	#include "test_libmem_0005-8.cc"
	#undef TYPE

	RETURN_STATUS;
}
