#include "testmocking.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static size_t testmocking_vsnprintf_remaining = 0;

/**
 * @brief Arm vsnprintf mock to fail for the next n calls
 *
 * @param[in] n Number of consecutive vsnprintf calls that must fail
 */
void testmocking_vsnprintf_fail_next(size_t n)
{
	testmocking_vsnprintf_remaining = n;
}

/**
 * @brief Disable vsnprintf mock unconditionally
 */
void testmocking_vsnprintf_disable(void)
{
	testmocking_vsnprintf_remaining = 0;
}

#ifndef EVIL_EMPIRE_OS
int __real_vsnprintf(
	char       *str,
	size_t      size,
	const char *format,
	va_list     args) __attribute__((format(printf,3,0)));

/**
 * @brief Linker-redirected entry point for vsnprintf
 *
 * When the mock is armed each call decrements the remaining counter and
 * returns -1. When the counter reaches zero the call is forwarded to libc
 *
 * @param[out] str Destination buffer passed by the caller
 * @param[in] size Destination buffer size
 * @param[in] format printf-style format string
 * @param[in] args Collected variadic argument list
 * @return vsnprintf-compatible result
 */
int __wrap_vsnprintf(
	char       *str,
	size_t      size,
	const char *format,
	va_list     args)
{
	if(testmocking_vsnprintf_remaining > 0)
	{
		testmocking_vsnprintf_remaining--;
		return -1;
	}

	return __real_vsnprintf(str,size,format,args);
}
#endif
