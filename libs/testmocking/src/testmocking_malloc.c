#include "testmocking.h"

#include <stdlib.h>

static size_t testmocking_malloc_remaining = 0;

/**
 * @brief Arm malloc mock to return NULL for the next n calls
 *
 * @param[in] n Number of consecutive malloc calls that must return NULL
 */
void testmocking_malloc_fail_next(size_t n)
{
	testmocking_malloc_remaining = n;
}

/**
 * @brief Disable malloc mock unconditionally
 */
void testmocking_malloc_disable(void)
{
	testmocking_malloc_remaining = 0;
}

#ifndef EVIL_EMPIRE_OS
void *__real_malloc(size_t size);

/**
 * @brief Linker-redirected entry point for malloc
 *
 * When the mock is armed each call decrements the remaining counter and
 * returns NULL. When the counter reaches zero the call is forwarded to libc
 *
 * @param[in] size Allocation size in bytes
 * @return Pointer returned by libc malloc, or NULL when the mock fires
 */
void *__wrap_malloc(size_t size)
{
	if(testmocking_malloc_remaining > 0)
	{
		testmocking_malloc_remaining--;
		return NULL;
	}

	return __real_malloc(size);
}
#endif
