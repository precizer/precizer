#include "testmocking.h"

#include <errno.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>

static size_t testmocking_gettimeofday_remaining = 0;
static size_t testmocking_localtime_r_remaining = 0;

/**
 * @brief Arm gettimeofday mock to fail for the next n calls
 *
 * @param[in] n Number of consecutive gettimeofday calls that must fail
 */
void testmocking_gettimeofday_fail_next(size_t n)
{
	testmocking_gettimeofday_remaining = n;
}

/**
 * @brief Disable gettimeofday mock unconditionally
 */
void testmocking_gettimeofday_disable(void)
{
	testmocking_gettimeofday_remaining = 0;
}

/**
 * @brief Arm localtime_r mock to fail for the next n calls
 *
 * @param[in] n Number of consecutive localtime_r calls that must fail
 */
void testmocking_localtime_r_fail_next(size_t n)
{
	testmocking_localtime_r_remaining = n;
}

/**
 * @brief Disable localtime_r mock unconditionally
 */
void testmocking_localtime_r_disable(void)
{
	testmocking_localtime_r_remaining = 0;
}

#ifndef EVIL_EMPIRE_OS
int __real_gettimeofday(
	struct timeval *tv,
	void           *tz);

/**
 * @brief Linker-redirected entry point for gettimeofday
 *
 * When the mock is armed each call decrements the remaining counter, sets
 * errno to EIO, and fails. When the counter reaches zero the call is forwarded
 * to libc
 *
 * @param[out] tv Time value destination passed by the caller
 * @param[in] tz Obsolete timezone argument passed by the caller
 * @return gettimeofday-compatible result
 */
int __wrap_gettimeofday(
	struct timeval *tv,
	void           *tz)
{
	if(testmocking_gettimeofday_remaining > 0)
	{
		testmocking_gettimeofday_remaining--;
		errno = EIO;
		return -1;
	}

	return __real_gettimeofday(tv,tz);
}

struct tm *__real_localtime_r(
	const time_t *timer,
	struct tm    *result);

/**
 * @brief Linker-redirected entry point for localtime_r
 *
 * When the mock is armed each call decrements the remaining counter and
 * returns NULL. When the counter reaches zero the call is forwarded to libc
 *
 * @param[in] timer Time value passed by the caller
 * @param[out] result Destination structure passed by the caller
 * @return localtime_r-compatible result
 */
struct tm *__wrap_localtime_r(
	const time_t *timer,
	struct tm    *result)
{
	if(testmocking_localtime_r_remaining > 0)
	{
		testmocking_localtime_r_remaining--;
		return NULL;
	}

	return __real_localtime_r(timer,result);
}
#endif
