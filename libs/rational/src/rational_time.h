/**
 *
 * @file
 * @brief Time Logging structures and functions prototypes
 *
 */

// Functions of working with time
#include <time.h>
#include <sys/time.h>

/// Structure to record a date
typedef struct {
	long long int nanoseconds;      // ns 1/1,000,000,000 of a second.
	long long int microseconds;     // μs 1/1,000,000 of a second.
	long long int milliseconds;     // ms 1/1000) of a second
	long long int seconds;
	long long int minutes;
	long long int hours;
	long long int days;
	long long int weeks;
	long long int months;
	long long int years;
} Date;

long long int cur_time_ns(void);

/**
 * @brief Current monotonic time in nanoseconds
 * @return long long int number of monotonic nanoseconds from an unspecified start point
 * @details Uses CLOCK_MONOTONIC when supported by the target platform.
 * If unavailable at compile time, the name transparently falls back to cur_time_ns().
 */
#if defined(CLOCK_MONOTONIC) && (!defined(_POSIX_MONOTONIC_CLOCK) || (_POSIX_MONOTONIC_CLOCK >= 0))
long long int cur_time_monotonic_ns(void);
#else
#define cur_time_monotonic_ns cur_time_ns
#endif

long long int cur_time_ms(void);

char *seconds_to_ISOdate(time_t seconds);

/**
 * @brief Convert nanoseconds to a human-readable date string in caller-provided buffer.
 * @param nanoseconds Time span in nanoseconds.
 * @param buffer Destination buffer.
 * @param buffer_size Destination buffer size in bytes.
 * @return @p buffer on success, NULL when @p buffer is NULL or @p buffer_size is zero.
 */
char *form_date_r(
	const long long int,
	char *,
	size_t);

char *form_date(const long long int);
