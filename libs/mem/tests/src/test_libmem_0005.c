#include "test_libmem_utils.h"

/**
 * @brief Multiple tests with unsigned long long int type and different array sizes
 *
 */
Return test_libmem_0005(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE unsigned long long int
	#include "test_libmem_0005-8.cc"
	#undef TYPE

	RETURN_STATUS;
}
