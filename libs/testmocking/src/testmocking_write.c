#include "testmocking.h"

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

static size_t testmocking_write_fail_remaining = 0;
static int testmocking_write_errno = EIO;

/**
 * @brief Arm write mock to fail for the next n calls
 *
 * @param[in] n Number of consecutive write calls that must fail
 * @param[in] err errno value exposed to the tested code
 */
void testmocking_write_fail_next(
	size_t n,
	int    err)
{
	testmocking_write_fail_remaining = n;
	testmocking_write_errno = err;
}

/**
 * @brief Disable write mock unconditionally
 */
void testmocking_write_disable(void)
{
	testmocking_write_fail_remaining = 0;
	testmocking_write_errno = EIO;
}

#ifndef EVIL_EMPIRE_OS
ssize_t __real_write(
	int        fd,
	const void *buf,
	size_t     count);

/**
 * @brief Linker-redirected entry point for write
 *
 * The mock is transparent by default. When a test arms it, the next selected
 * calls fail with the configured errno value so low-level error-reporting
 * fallback paths can be exercised deterministically
 *
 * @param[in] fd File descriptor passed by the caller
 * @param[in] buf Source buffer passed by the caller
 * @param[in] count Number of bytes requested by the caller
 * @return write-compatible result
 */
ssize_t __wrap_write(
	int        fd,
	const void *buf,
	size_t     count)
{
	if(testmocking_write_fail_remaining > 0U)
	{
		testmocking_write_fail_remaining--;
		errno = testmocking_write_errno;
		return(-1);
	}

	return(__real_write(fd,buf,count));
}
#endif
