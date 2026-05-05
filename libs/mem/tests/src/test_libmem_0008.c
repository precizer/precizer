#include "test_libmem_utils.h"

/**
 * @brief Multiple tests with unsigned char type and different array sizes
 *
 */
Return test_libmem_0008(void)
{
	INITTEST;

	SLOWTEST;

	#define TYPE unsigned char
	#include "test_libmem_0005-8.cc"
	#undef TYPE

	RETURN_STATUS;
}
