#include "testmocking.h"

#include <stdlib.h>

static size_t testmocking_realloc_remaining = 0;

/**
 * @brief Arm realloc mock to return NULL for the next n calls
 *
 * @param[in] n Number of consecutive realloc calls that must return NULL
 */
void testmocking_realloc_fail_next(size_t n)
{
	testmocking_realloc_remaining = n;
}

/**
 * @brief Disable realloc mock unconditionally
 */
void testmocking_realloc_disable(void)
{
	testmocking_realloc_remaining = 0;
}

#ifndef EVIL_EMPIRE_OS
void *__real_realloc(void *ptr,size_t size);

/**
 * @brief Linker-redirected entry point for realloc
 *
 * When the mock is armed each call decrements the remaining counter and
 * returns NULL. When the counter reaches zero the call is forwarded to libc
 *
 * @param[in] ptr Existing allocation passed through to libc realloc
 * @param[in] size New allocation size in bytes
 * @return Pointer returned by libc realloc, or NULL when the mock fires
 */
void *__wrap_realloc(void *ptr,size_t size)
{
	if(testmocking_realloc_remaining > 0)
	{
		testmocking_realloc_remaining--;
		return NULL;
	}

	return __real_realloc(ptr,size);
}
#endif
