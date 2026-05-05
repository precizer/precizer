#include "test_libmem_utils.h"

/**
 * @brief Multiple tests with int type and different array sizes
 *
 */
Return test_libmem_0007(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE int
	#include "test_libmem_0005-8.cc"
	#undef TYPE

	RETURN_STATUS;
}
