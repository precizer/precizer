#include "sute.h"
#include "mocks_librational.h"

/**
 * @brief Convert a struct timeval into integer milliseconds
 *
 * @param value Timeval whose seconds and microseconds are folded into a single ms count
 * @return Milliseconds since the same epoch the input refers to
 */
static long long int timeval_to_ms(
	const struct timeval *const value)
{
	return((long long int)value->tv_sec * 1000LL + (long long int)value->tv_usec / 1000LL);
}

/**
 * @brief Convert a struct timespec into integer nanoseconds
 *
 * @param value Timespec whose seconds and nanoseconds are folded into a single ns count
 * @return Nanoseconds since the same epoch the input refers to
 */
static long long int timespec_to_ns(
	const struct timespec *const value)
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

static Return test_librational_0003_3(void)
{
	INITTEST;

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

static Return test_librational_0003_5(void)
{
	INITTEST;

	const long long int ns_in_year = 31536000000000000LL;
	const long long int ns_in_month = 2628000000000000LL;
	const long long int ns_in_week = 604800000000000LL;
	const long long int ns_in_day = 86400000000000LL;
	const long long int ns_in_minute = 60000000000LL;
	const long long int ns_in_second = 1000000000LL;

	/* Exercise the MAJOR_VIEW decision chain for units not reached by shorter tests */
	ASSERT(0 == strcmp(form_date(ns_in_year,MAJOR_VIEW),"1y"));
	ASSERT(0 == strcmp(form_date(ns_in_month,MAJOR_VIEW),"1mon"));
	ASSERT(0 == strcmp(form_date(ns_in_week,MAJOR_VIEW),"1w"));
	ASSERT(0 == strcmp(form_date(ns_in_day,MAJOR_VIEW),"1d"));
	ASSERT(0 == strcmp(form_date(ns_in_minute,MAJOR_VIEW),"1min"));
	ASSERT(0 == strcmp(form_date(ns_in_second,MAJOR_VIEW),"1s"));

	RETURN_STATUS;
}

static Return test_librational_0003_6(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	char date_buffer[MAX_CHARACTERS] = {0};

	/* A failed snprintf inside catdate_r() leaves the caller buffer empty */
	mocks_librational_snprintf_fail_next(1);
	const char *formatted_date = form_date_r(
		1000LL,
		FULL_VIEW,
		date_buffer,
		sizeof(date_buffer));
	mocks_librational_disable();

	ASSERT(formatted_date == date_buffer);
	ASSERT(date_buffer[0] == '\0');

	RETURN_STATUS;
}

static Return test_librational_0003_7(void)
{
	INITTEST;

	SKIP_ON_EVIL_EMPIRE_OS;

	char date_buffer[8] = {0};

	/* Truncation marks the destination as full and preserves NUL termination */
	mocks_librational_snprintf_truncate_next(1);
	const char *formatted_date = form_date_r(
		1001LL,
		FULL_VIEW,
		date_buffer,
		sizeof(date_buffer));
	mocks_librational_disable();

	ASSERT(formatted_date == date_buffer);
	ASSERT(date_buffer[0] == 'T');
	ASSERT(date_buffer[sizeof(date_buffer) - 1U] == '\0');

	RETURN_STATUS;
}

static Return test_librational_0003_8(void)
{
	INITTEST;

	/* The monotonic helper is for intervals, so only ordering and positivity are asserted */
	const long long int before_ns = cur_time_monotonic_ns();
	const long long int after_ns = cur_time_monotonic_ns();

	ASSERT(before_ns > 0LL);
	ASSERT(after_ns > 0LL);
	ASSERT(after_ns >= before_ns);

	RETURN_STATUS;
}

static Return test_librational_0003_9(void)
{
	INITTEST;

	const char *iso_date = seconds_to_ISOdate(0);

	/* seconds_to_ISOdate() returns a stable static buffer in YYYY-MM-DD HH:MM:SS shape */
	ASSERT(iso_date != NULL);
	ASSERT(strlen(iso_date) == strlen("1970-01-01 00:00:00"));
	ASSERT(iso_date[4] == '-');
	ASSERT(iso_date[7] == '-');
	ASSERT(iso_date[10] == ' ');
	ASSERT(iso_date[13] == ':');
	ASSERT(iso_date[16] == ':');

	RETURN_STATUS;
}

/**
 * @brief Run librational time and duration helper tests
 *
 * The suite verifies the time-related public API of librational. It checks
 * that cur_time_ms() and cur_time_ns() return positive epoch values, use the
 * documented time units, and agree with each other within a tolerance
 * suitable for wall-clock calls; that cur_time_monotonic_ns() returns
 * non-decreasing values over a measurable interval; that form_date() and
 * form_date_r() decompose nanoseconds into a human-readable string, select
 * the largest unit in MAJOR_VIEW, and survive snprintf failures and
 * truncation injected through the link-time mocks; that seconds_to_ISOdate()
 * returns a fixed-width ISO-like timestamp from a stable static buffer; and
 * that form_date_r() leaves the destination empty for negative input
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

	TEST(test_librational_0003_1,"cur_time_ms() returns current epoch milliseconds…");
	TEST(test_librational_0003_2,"cur_time_ns() returns current epoch nanoseconds…");
	TEST(test_librational_0003_3,"cur_time_ms() and cur_time_ns() use the same wall-clock epoch…");
	TEST(test_librational_0003_4,"form_date_r() formats a complete duration decomposition…");
	TEST(test_librational_0003_5,"form_date() selects the largest requested duration unit…");
	TEST(test_librational_0003_6,"form_date_r() tolerates snprintf failure inside duration assembly…");
	TEST(test_librational_0003_7,"form_date_r() keeps truncated duration buffers terminated…");
	TEST(test_librational_0003_8,"cur_time_monotonic_ns() returns ordered monotonic nanoseconds…");
	TEST(test_librational_0003_9,"seconds_to_ISOdate() returns a fixed-width ISO-like timestamp…");
	TEST(test_librational_0003_10,"form_date_r() returns an empty string for negative input…");

	RETURN_STATUS;
}
