#include "test_librational_all.h"
#include "testmocking.h"

/**
 * @brief Convert a struct timeval into integer milliseconds
 *
 * @param value Timeval whose seconds and microseconds are folded into a single ms count
 * @return Milliseconds since the same epoch the input refers to
 */
static long long int timeval_to_ms(const struct timeval *const value)
{
	return((long long int)value->tv_sec * 1000LL + (long long int)value->tv_usec / 1000LL);
}

/**
 * @brief Convert a struct timespec into integer nanoseconds
 *
 * @param value Timespec whose seconds and nanoseconds are folded into a single ns count
 * @return Nanoseconds since the same epoch the input refers to
 */
static long long int timespec_to_ns(const struct timespec *const value)
{
	return((long long int)value->tv_sec * 1000000000LL + (long long int)value->tv_nsec);
}

/**
 * @brief Check that a value falls within a tolerated window around a range
 *
 * @details Returns true when value lies in [lower_bound - tolerance, upper_bound + tolerance].
 * The signed addition is safe for the wall-clock numbers exercised by this suite
 * (microseconds and nanoseconds since the Unix epoch sit far below LLONG_MAX),
 * but the helper is not suitable for arbitrary near-LLONG_MAX inputs
 *
 * @param value Observed sample
 * @param lower_bound Lower edge of the expected range
 * @param upper_bound Upper edge of the expected range
 * @param tolerance Slack added on each side of the range
 * @return `true` when @p value is inside the tolerated window
 */
static bool inside_tolerated_range(
	const long long int value,
	const long long int lower_bound,
	const long long int upper_bound,
	const long long int tolerance)
{
	return(value + tolerance >= lower_bound && value <= upper_bound + tolerance);
}

/**
 * @brief Check that cur_time_ms() returns wall-clock milliseconds
 *
 * @details Wraps the call between two gettimeofday() samples and verifies
 * that the observed value falls within the bracketed range with a tolerance
 * suitable for adjustable realtime clocks
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_1(void)
{
	INITTEST;

	const long long int tolerance_ms = 1000LL;
	struct timeval before = {0};
	struct timeval after = {0};

	/* Compare cur_time_ms() with gettimeofday() around the call */
	ASSERT(0 == gettimeofday(&before,NULL));
	const long long int observed_ms = cur_time_ms();
	ASSERT(0 == gettimeofday(&after,NULL));

	const long long int lower_ms = timeval_to_ms(&before);
	const long long int upper_ms = timeval_to_ms(&after);

	ASSERT(observed_ms > 0LL);
	ASSERT(inside_tolerated_range(observed_ms,lower_ms,upper_ms,tolerance_ms));

	RETURN_STATUS;
}

/**
 * @brief Check that cur_time_ns() returns wall-clock nanoseconds
 *
 * @details Wraps the call between two clock_gettime(CLOCK_REALTIME) samples
 * and verifies that the observed value falls within the bracketed range with
 * a tolerance suitable for adjustable realtime clocks
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_2(void)
{
	INITTEST;

	const long long int tolerance_ns = 1000000000LL;
	struct timespec before = {0};
	struct timespec after = {0};

	/* Compare cur_time_ns() with CLOCK_REALTIME around the call */
	ASSERT(0 == clock_gettime(CLOCK_REALTIME,&before));
	const long long int observed_ns = cur_time_ns();
	ASSERT(0 == clock_gettime(CLOCK_REALTIME,&after));

	const long long int lower_ns = timespec_to_ns(&before);
	const long long int upper_ns = timespec_to_ns(&after);

	ASSERT(observed_ns > 0LL);
	ASSERT(inside_tolerated_range(observed_ns,lower_ns,upper_ns,tolerance_ns));

	RETURN_STATUS;
}

/**
 * @brief Check that cur_time_ms() and cur_time_ns() share the same epoch
 *
 * @details Brackets a cur_time_ns() sample with two cur_time_ms() calls and
 * verifies that the nanosecond reading, converted to milliseconds, lands
 * inside the bracketed range. This protects against silent drift between
 * the two helpers if their clock sources ever diverge
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_3(void)
{
	INITTEST;

	/* Both helpers map to CLOCK_REALTIME under the hood, so their numerical drift is
	   on the order of nanoseconds. The 1-second tolerance is therefore not a numerical
	   slack but a safety margin against a wall-clock adjustment (NTP step, manual set)
	   landing exactly between the three samples taken below */
	const long long int tolerance_ms = 1000LL;

	/* cur_time_ns() should represent the same wall-clock epoch as cur_time_ms() */
	const long long int before_ms = cur_time_ms();
	const long long int observed_ns = cur_time_ns();
	const long long int after_ms = cur_time_ms();
	const long long int observed_ns_as_ms = observed_ns / 1000000LL;

	ASSERT(before_ms > 0LL);
	ASSERT(observed_ns > 0LL);
	ASSERT(after_ms > 0LL);
	ASSERT(inside_tolerated_range(observed_ns_as_ms,before_ms,after_ms,tolerance_ms));

	RETURN_STATUS;
}

/**
 * @brief Check the complete duration decomposition through form_date_r()
 *
 * @details Uses one carefully constructed nanosecond value that exercises every
 * non-zero unit (years, months, weeks, days, hours, minutes, seconds,
 * milliseconds, microseconds, nanoseconds) in a single FULL_VIEW string. Also
 * verifies pointer equality with the caller-provided buffer and that the
 * static-buffer wrapper form_date() yields the same text
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_4(void)
{
	INITTEST;

	const long long int full_date_ns = 339800645368118513LL;
	const char expected_full_date[] = "10y 9mon 1w 2d 3h 4min 5s 368ms 118μs 513ns";
	char date_buffer[MAX_CHARACTERS] = {0};

	/* Verify a complete decomposition through both public formatting variants */
	const char *formatted_date = form_date_r(
		full_date_ns,
		FULL_VIEW,
		date_buffer,
		sizeof(date_buffer));

	ASSERT(formatted_date == date_buffer);
	ASSERT(0 == strcmp(date_buffer,expected_full_date));
	ASSERT(0 == strcmp(form_date(full_date_ns,FULL_VIEW),expected_full_date));

	RETURN_STATUS;
}

/**
 * @brief Check MAJOR_VIEW unit selection across the full unit ladder
 *
 * @details Feeds form_date() exactly one nanosecond count per unit and confirms
 * that MAJOR_VIEW renders only the matching unit. Covers every branch of the
 * if-else chain in form_date_r() so any future regression in unit ordering
 * is caught here rather than at higher-level usage sites
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_5(void)
{
	INITTEST;

	const long long int ns_in_year = 31536000000000000LL;
	const long long int ns_in_month = 2628000000000000LL;
	const long long int ns_in_week = 604800000000000LL;
	const long long int ns_in_day = 86400000000000LL;
	const long long int ns_in_hour = 3600000000000LL;
	const long long int ns_in_minute = 60000000000LL;
	const long long int ns_in_second = 1000000000LL;
	const long long int ns_in_millisecond = 1000000LL;
	const long long int ns_in_microsecond = 1000LL;

	/* Exercise every branch of the MAJOR_VIEW decision chain so unit ordering regressions surface here */
	ASSERT(0 == strcmp(form_date(ns_in_year,MAJOR_VIEW),"1y"));
	ASSERT(0 == strcmp(form_date(ns_in_month,MAJOR_VIEW),"1mon"));
	ASSERT(0 == strcmp(form_date(ns_in_week,MAJOR_VIEW),"1w"));
	ASSERT(0 == strcmp(form_date(ns_in_day,MAJOR_VIEW),"1d"));
	ASSERT(0 == strcmp(form_date(ns_in_hour,MAJOR_VIEW),"1h"));
	ASSERT(0 == strcmp(form_date(ns_in_minute,MAJOR_VIEW),"1min"));
	ASSERT(0 == strcmp(form_date(ns_in_second,MAJOR_VIEW),"1s"));
	ASSERT(0 == strcmp(form_date(ns_in_millisecond,MAJOR_VIEW),"1ms"));
	ASSERT(0 == strcmp(form_date(ns_in_microsecond,MAJOR_VIEW),"1μs"));
	ASSERT(0 == strcmp(form_date(1LL,MAJOR_VIEW),"1ns"));

	RETURN_STATUS;
}

#ifndef EVIL_EMPIRE_OS
/**
 * @brief Check that form_date_r() tolerates a failing snprintf
 *
 * @details Arms the link-time snprintf mock to return -1 for the next call,
 * invokes form_date_r() and verifies that the caller buffer is left as an
 * empty terminated string. Evil Empire OS builds exclude this subtest because
 * the mocks rely on GNU ld --wrap
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_6(void)
{
	INITTEST;

	/* The link-time snprintf mock used below relies on GNU ld --wrap */

	char date_buffer[MAX_CHARACTERS] = {0};

	/* A failed snprintf inside catdate_r() leaves the caller buffer empty */
	testmocking_snprintf_fail_next(1);
	const char *formatted_date = form_date_r(
		1000LL,
		FULL_VIEW,
		date_buffer,
		sizeof(date_buffer));
	testmocking_snprintf_disable();

	ASSERT(formatted_date == date_buffer);
	ASSERT(date_buffer[0] == '\0');

	RETURN_STATUS;
}

/**
 * @brief Check that form_date_r() keeps truncated buffers terminated
 *
 * @details Arms the link-time snprintf mock to report truncation for the next
 * call. The mock writes a sentinel 'T' at buffer[0] and reports a length that
 * forces the production code onto the truncation path. The assertions verify
 * that the destination is filled up to its last byte and remains NUL
 * terminated. Evil Empire OS builds exclude this subtest because the mocks
 * rely on GNU ld --wrap
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_7(void)
{
	INITTEST;

	/* The link-time snprintf mock used below relies on GNU ld --wrap */

	char date_buffer[8] = {0};

	/* Truncation marks the destination as full and preserves NUL termination */
	testmocking_snprintf_truncate_next(1);
	const char *formatted_date = form_date_r(
		1001LL,
		FULL_VIEW,
		date_buffer,
		sizeof(date_buffer));
	testmocking_snprintf_disable();

	ASSERT(formatted_date == date_buffer);
	ASSERT(date_buffer[0] == 'T');
	ASSERT(date_buffer[sizeof(date_buffer) - 1U] == '\0');

	RETURN_STATUS;
}
#endif

/**
 * @brief Check that cur_time_monotonic_ns() returns ordered samples around a measurable interval
 *
 * @details Two calls bracket a short nanosleep(). The interval must be strictly
 * positive (rules out a stuck clock or a wrong unit) and bounded above by a
 * generous ceiling that tolerates loaded CI runners without becoming a flaky
 * test. On platforms without CLOCK_MONOTONIC, cur_time_monotonic_ns() falls
 * back to cur_time_ns(), so this test checks the public helper contract rather
 * than proving that a monotonic clock source exists. Positivity of the absolute
 * values protects against accidental zero initialization
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_8(void)
{
	INITTEST;

	/* A measurable sleep between samples lets the test assert a strictly positive interval */
	const long long int before_ns = cur_time_monotonic_ns();
	const struct timespec sleep_request = { .tv_sec = 0,.tv_nsec = 1000000LL };
	(void)nanosleep(&sleep_request,NULL);
	const long long int after_ns = cur_time_monotonic_ns();

	ASSERT(before_ns > 0LL);
	ASSERT(after_ns > 0LL);

	const long long int delta_ns = after_ns - before_ns;

	/* Upper bound is intentionally generous (10 seconds) to keep the test reliable under load */
	ASSERT(delta_ns > 0LL);
	ASSERT(delta_ns < 10000000000LL);

	RETURN_STATUS;
}

/**
 * @brief Check seconds_to_ISOdate() shape, character classes and static-buffer overwrite behavior
 *
 * @details Verifies that the returned text has the fixed
 * "YYYY-MM-DD HH:MM:SS" shape, that every position the format reserves for a
 * decimal digit actually contains a digit, that the separator characters land
 * on the documented positions, that the function returns a stable static buffer
 * (the same pointer on consecutive calls), and that a later call overwrites the
 * same storage with text for the new input
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_9(void)
{
	INITTEST;

	const char *iso_date = seconds_to_ISOdate(0);
	char first_iso_date[sizeof "1970-01-01 00:00:00"];

	/* seconds_to_ISOdate() returns a stable static buffer in YYYY-MM-DD HH:MM:SS shape */
	ASSERT(iso_date != NULL);
	ASSERT(strlen(iso_date) == strlen("1970-01-01 00:00:00"));
	memcpy(first_iso_date,iso_date,sizeof(first_iso_date));
	ASSERT(iso_date[4] == '-');
	ASSERT(iso_date[7] == '-');
	ASSERT(iso_date[10] == ' ');
	ASSERT(iso_date[13] == ':');
	ASSERT(iso_date[16] == ':');

	/* Positions reserved for decimal digits in YYYY-MM-DD HH:MM:SS must hold a digit.
	   The indices below are tied to the exact format string used by seconds_to_ISOdate()
	   (currently "%Y-%m-%d %H:%M:%S") — any change to that strftime() format must be
	   mirrored here, otherwise this assertion will either miss new digit slots or
	   wrongly flag separators */
	const size_t digit_positions[] = {0U,1U,2U,3U,5U,6U,8U,9U,11U,12U,14U,15U,17U,18U};

	for(size_t i = 0U; i < sizeof(digit_positions) / sizeof(digit_positions[0]); i++)
	{
		const char ch = iso_date[digit_positions[i]];
		ASSERT(ch >= '0' && ch <= '9');
	}

	/* The documented stable static buffer must be reused across consecutive calls */
	const char *iso_date_again = seconds_to_ISOdate(86400);
	ASSERT(iso_date_again == iso_date);
	ASSERT(0 != strcmp(iso_date_again,first_iso_date));

	RETURN_STATUS;
}

/**
 * @brief Check that form_date_r() rejects invalid output buffers
 *
 * @details Covers the public validation contract for the reentrant duration
 * formatter: a NULL destination or zero-sized destination is rejected with a
 * NULL return value
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_10(void)
{
	INITTEST;

	char date_buffer[MAX_CHARACTERS] = "unchanged";

	/* Invalid output buffer arguments must be rejected before formatting starts */
	ASSERT(NULL == form_date_r(1LL,FULL_VIEW,NULL,sizeof(date_buffer)));
	ASSERT(NULL == form_date_r(1LL,FULL_VIEW,date_buffer,0U));
	ASSERT(0 == strcmp(date_buffer,"unchanged"));

	RETURN_STATUS;
}

/**
 * @brief Check that form_date_r() returns an empty string for negative input
 *
 * @details The library does not currently document a meaningful rendering for
 * negative nanosecond counts. asadate() produces non-positive components for
 * negative inputs, catdate_r() skips non-positive numbers, and the early
 * zero-handling branch only matches exactly nanoseconds == 0. The test pins
 * the current behaviour (empty terminated string for both FULL_VIEW and
 * MAJOR_VIEW) so any unintended change is detected immediately
 *
 * @return Return describing success or failure
 */
static Return test_librational_0003_11(void)
{
	INITTEST;

	char date_buffer[MAX_CHARACTERS];

	/* Sentinel fill verifies that form_date_r() actually wrote the terminator on the negative-input path */
	memset(date_buffer,'X',sizeof(date_buffer));
	const char *formatted_full = form_date_r(-1LL,FULL_VIEW,date_buffer,sizeof(date_buffer));
	ASSERT(formatted_full == date_buffer);
	ASSERT(date_buffer[0] == '\0');

	memset(date_buffer,'Y',sizeof(date_buffer));
	const char *formatted_major = form_date_r(-1LL,MAJOR_VIEW,date_buffer,sizeof(date_buffer));
	ASSERT(formatted_major == date_buffer);
	ASSERT(date_buffer[0] == '\0');

	RETURN_STATUS;
}

/**
 * @brief Run librational time and duration helper tests
 *
 * The suite verifies the time-related public API of librational. It checks
 * that cur_time_ms() and cur_time_ns() return positive epoch values, use the
 * documented time units, and agree with each other within a tolerance
 * suitable for wall-clock calls; that cur_time_monotonic_ns() or its fallback
 * returns ordered values over a measurable interval; that form_date() and
 * form_date_r() decompose nanoseconds into a human-readable string, select
 * the largest unit in MAJOR_VIEW, reject invalid output buffers, and survive
 * snprintf failures and truncation injected through the link-time mocks; that
 * seconds_to_ISOdate() returns a fixed-width ISO-like timestamp from a stable
 * static buffer and overwrites that buffer on later calls; and that
 * form_date_r() leaves the destination empty for negative input
 *
 * Covered API surface:
 * - cur_time_ms(), cur_time_ns(), cur_time_monotonic_ns()
 * - form_date(), form_date_r()
 * - seconds_to_ISOdate()
 *
 * @return Return describing success or failure
 */
Return test_librational_0003(void)
{
	INITTEST;

	TEST(test_librational_0003_1,"cur_time_ms() returns current epoch milliseconds");
	TEST(test_librational_0003_2,"cur_time_ns() returns current epoch nanoseconds");
	TEST(test_librational_0003_3,"cur_time_ms() and cur_time_ns() use the same wall-clock epoch");
	TEST(test_librational_0003_4,"form_date_r() formats a complete duration decomposition");
	TEST(test_librational_0003_5,"form_date() selects the largest requested duration unit");
#ifndef EVIL_EMPIRE_OS
	TEST(test_librational_0003_6,"form_date_r() tolerates snprintf failure inside duration assembly");
	TEST(test_librational_0003_7,"form_date_r() keeps truncated duration buffers terminated");
#endif
	TEST(test_librational_0003_8,"cur_time_monotonic_ns() returns ordered nanosecond samples");
	TEST(test_librational_0003_9,"seconds_to_ISOdate() returns and overwrites a fixed-width static timestamp");
	TEST(test_librational_0003_10,"form_date_r() rejects invalid output buffers");
	TEST(test_librational_0003_11,"form_date_r() returns an empty string for negative input");

	RETURN_STATUS;
}
