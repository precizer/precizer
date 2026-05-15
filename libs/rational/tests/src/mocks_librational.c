#include "mocks_librational.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

/* Counters of remaining injected libc outcomes. They default to zero so
   the mock is transparent until a subtest arms one of the paths */
static size_t mock_snprintf_fail_remaining = 0;
static size_t mock_snprintf_truncate_remaining = 0;
static size_t mock_write_fail_remaining = 0;
static int mock_write_errno = EIO;

/**
 * @brief Arm the snprintf mock to return -1 for the next n calls
 *
 * @param[in] n Number of consecutive snprintf calls that must fail
 */
void mocks_librational_snprintf_fail_next(size_t n)
{
	mock_snprintf_fail_remaining = n;
}

/**
 * @brief Arm the snprintf mock to report truncation for the next n calls
 *
 * @param[in] n Number of consecutive snprintf calls that must report truncation
 */
void mocks_librational_snprintf_truncate_next(size_t n)
{
	mock_snprintf_truncate_remaining = n;
}

/**
 * @brief Arm the write mock to fail for the next n calls
 *
 * @param[in] n Number of consecutive write calls that must fail
 * @param[in] err errno value exposed to the tested code
 */
void mocks_librational_write_fail_next(size_t n,int err)
{
	mock_write_fail_remaining = n;
	mock_write_errno = err;
}

/**
 * @brief Disable all librational mocks
 */
void mocks_librational_disable(void)
{
	mock_snprintf_fail_remaining = 0;
	mock_snprintf_truncate_remaining = 0;
	mock_write_fail_remaining = 0;
	mock_write_errno = EIO;
}

int __wrap_snprintf(
	char       *str,
	size_t      size,
	const char *format,
	...) __attribute__((format(printf,3,4)));

/**
 * @brief Linker-redirected entry point for snprintf
 *
 * When the mock is armed, it can inject either a formatting failure or a
 * truncation result. Otherwise the call is forwarded to libc through
 * vsnprintf, which is the standard way to pass a collected variadic argument
 * list to a printf-family implementation
 *
 * @param[out] str Destination buffer passed by the caller
 * @param[in] size Destination buffer size
 * @param[in] format printf-style format string
 * @return snprintf-compatible result
 */
int __wrap_snprintf(
	char       *str,
	size_t      size,
	const char *format,
	...)
{
	if(mock_snprintf_fail_remaining > 0U)
	{
		mock_snprintf_fail_remaining--;
		return(-1);
	}

	if(mock_snprintf_truncate_remaining > 0U)
	{
		mock_snprintf_truncate_remaining--;

		if(str != NULL && size > 0U)
		{
			str[0] = 'T';

			if(size > 1U)
			{
				str[1] = '\0';
			}
		}

		int reported_length = 1;

		if(size < (size_t)INT_MAX)
		{
			reported_length = (int)size;
		} else {
			reported_length = INT_MAX;
		}

		if(reported_length == 0)
		{
			reported_length = 1;
		}

		return(reported_length);
	}

	va_list args;
	va_start(args,format);
	const int result = vsnprintf(str,size,format,args);
	va_end(args);

	return(result);
}

ssize_t __real_write(
	int         fd,
	const void *buf,
	size_t      count);

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
	int         fd,
	const void *buf,
	size_t      count)
{
	if(mock_write_fail_remaining > 0U)
	{
		mock_write_fail_remaining--;
		errno = mock_write_errno;
		return(-1);
	}

	return(__real_write(fd,buf,count));
}
