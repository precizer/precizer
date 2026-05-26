#include "sute.h"
#include "testmocking.h"

#include <errno.h>
#include <float.h>

/**
 * @brief Copy a formatted number without thousands separators
 *
 * @param src Source string that may contain comma separators
 * @param dst Destination buffer
 */
static void strip_commas(
	const char *src,
	char       *dst)
{
	size_t write = 0U;

	for(size_t read = 0U; src[read] != '\0'; read++)
	{
		if(src[read] == ',')
		{
			continue;
		}

		dst[write++] = src[read];
	}

	dst[write] = '\0';
}

/**
 * @brief Check comma grouping in an integer string
 *
 * @param val String to validate
 * @return `true` when @p val uses valid groups of three digits
 */
static bool valid_comma_grouping(const char *val)
{
	size_t start = 0U;

	if(val[0] == '-')
	{
		start = 1U;
	}

	size_t len = strlen(val);

	size_t digits_since_comma = 0U;

	for(size_t i = len; i > start; i--)
	{
		const char ch = val[i - 1U];

		if(ch >= '0' && ch <= '9')
		{
			digits_since_comma++;
			continue;
		}

		if(ch == ',')
		{
			if(digits_since_comma != 3U)
			{
				return false;
			}

			digits_since_comma = 0U;
			continue;
		}

		return false;
	}

	return digits_since_comma >= 1U && digits_since_comma <= 3U;
}

/**
 * @brief Compare a signed formatted integer with the standard ungrouped value
 *
 * @param val Integer value that was formatted
 * @param formatted Candidate formatted string with comma grouping
 * @return `true` when @p formatted has valid grouping and preserves the value
 */
static bool form_intmax_matches_standard(
	intmax_t   val,
	const char *formatted)
{
	char expected[MAX_CHARACTERS];
	char stripped[MAX_CHARACTERS];

	(void)snprintf(expected,sizeof(expected),"%" PRIdMAX,val);

	if(valid_comma_grouping(formatted) == false)
	{
		return false;
	}

	strip_commas(formatted,stripped);

	return 0 == strcmp(stripped,expected);
}

/**
 * @brief Compare an unsigned formatted integer with the standard ungrouped value
 *
 * @param val Integer value that was formatted
 * @param formatted Candidate formatted string with comma grouping
 * @return `true` when @p formatted has valid grouping and preserves the value
 */
static bool form_uintmax_matches_standard(
	uintmax_t  val,
	const char *formatted)
{
	char expected[MAX_CHARACTERS];
	char stripped[MAX_CHARACTERS];

	(void)snprintf(expected,sizeof(expected),"%" PRIuMAX,val);

	if(valid_comma_grouping(formatted) == false)
	{
		return false;
	}

	strip_commas(formatted,stripped);

	return 0 == strcmp(stripped,expected);
}

/**
 * @brief Validate the grouped integer part of a formatted real number
 *
 * @param val Full formatted real string
 * @param start First character index of the integer part
 * @param end One-past-last character index of the integer part
 * @return `true` when the integer part uses valid comma grouping
 */
static bool valid_grouped_integer_part(
	const char *val,
	size_t     start,
	size_t     end)
{
	size_t digits_since_comma = 0U;

	for(size_t i = end; i > start; i--)
	{
		const char ch = val[i - 1U];

		if(ch >= '0' && ch <= '9')
		{
			digits_since_comma++;
			continue;
		}

		if(ch == ',')
		{
			if(digits_since_comma != 3U)
			{
				return false;
			}

			digits_since_comma = 0U;
			continue;
		}

		return false;
	}

	return digits_since_comma >= 1U && digits_since_comma <= 3U;
}

/**
 * @brief Validate portable formatting rules for a grouped real number string
 *
 * @param val Formatted real string
 * @return `true` when @p val has valid grouping and fractional text
 */
static bool valid_grouped_real(const char *val)
{
	size_t start = 0U;

	if(val[0] == '-')
	{
		start = 1U;
	}

	const char *dot = strchr(val + start,'.');
	const size_t total_len = strlen(val);
	const size_t integer_end = dot == NULL ? total_len : (size_t)(dot - val);

	if(valid_grouped_integer_part(val,start,integer_end) == false)
	{
		return false;
	}

	if(dot == NULL)
	{
		return true;
	}

	const char *fraction = dot + 1U;

	if(*fraction == '\0')
	{
		return false;
	}

	for(size_t i = 0U; fraction[i] != '\0'; i++)
	{
		if(fraction[i] < '0' || fraction[i] > '9')
		{
			return false;
		}
	}

	return fraction[strlen(fraction) - 1U] != '0';
}

/**
 * @brief Check a formatted real number without depending on exact long double text
 *
 * @param expected Numeric value that was formatted
 * @param formatted Candidate formatted string
 * @return `true` when @p formatted parses back close enough to @p expected
 */
static bool form_real_matches_portable(
	long double expected,
	const char  *formatted)
{
	if(valid_grouped_real(formatted) == false)
	{
		return false;
	}

	char stripped[MAX_CHARACTERS];
	strip_commas(formatted,stripped);

	errno = 0;
	const long double parsed = strtold(stripped,NULL);

	if(errno != 0)
	{
		return false;
	}

	long double tolerance = fabsl(expected) * LDBL_EPSILON * 4.0L;

	if(tolerance < 0.000000001L)
	{
		tolerance = 0.000000001L;
	}
	return fabsl(parsed - expected) <= tolerance;
}

/**
 * @brief Check real-number formatting through the generic form() macro
 *
 * @details Exercises stable text expectations (zero handling, near-zero
 * normalization where magnitudes below 1e-10 collapse to "0", rounding
 * to at most 9 fractional digits, grouped integer parts using ',' as
 * thousands separator and '.' as decimal point regardless of the
 * current locale, sign placement, and stripping of trailing fractional
 * zeros) and portable equivalence for long double values whose exact
 * decimal text is platform dependent — those are parsed back through
 * strtold() and compared against the original value with an LDBL_EPSILON
 * tolerance. Also verifies that the generic dispatch correctly routes
 * plain double and float arguments to the real formatter, and that a
 * successful call returns the caller-provided buffer pointer (not an
 * internal static or a literal "")
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_1(void)
{
	INITTEST;

	char formatted[MAX_CHARACTERS];

	/* The thousands separator ',' and decimal point '.' are fixed by the library and not affected by the current locale */

	/* Exact checks cover zero handling, near-zero normalization, rounding, and grouped integer parts */
	ASSERT(0 == strcmp(form(0.0L,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form(-0.0000000004L,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form(0.0000000006L,formatted,sizeof(formatted)),"0.000000001"));
	ASSERT(0 == strcmp(form(-0.0000000006L,formatted,sizeof(formatted)),"-0.000000001"));
	ASSERT(0 == strcmp(form(1234567.0L,formatted,sizeof(formatted)),"1,234,567"));
	ASSERT(0 == strcmp(form(1234567890123456.0L,formatted,sizeof(formatted)),"1,234,567,890,123,456"));
	ASSERT(0 == strcmp(form(1234567.125L,formatted,sizeof(formatted)),"1,234,567.125"));
	ASSERT(0 == strcmp(form(-9876.5L,formatted,sizeof(formatted)),"-9,876.5"));

	/* Portable checks avoid tying the test to one exact long double decimal spelling */
	ASSERT(form_real_matches_portable(123456789.123456780L,form(123456789.123456780L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(987654321098.123456789L,form(987654321098.123456789L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(1.23L,form(1.23L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(9.9999999995L,form(9.9999999995L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(-9.9999999995L,form(-9.9999999995L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(999.9999999995L,form(999.9999999995L,formatted,sizeof(formatted))));

	/* The generic form() macro must also route plain double and float arguments to the real formatter.
	 * The chosen values are exactly representable in float so the textual result remains stable */
	ASSERT(0 == strcmp(form((double)0.0,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form((double)1234.5,formatted,sizeof(formatted)),"1,234.5"));
	ASSERT(0 == strcmp(form((float)0.0F,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form((float)1234.5F,formatted,sizeof(formatted)),"1,234.5"));

	/* A successful call must return the caller-provided buffer pointer, not an internal static or literal */
	ASSERT(form(1.5L,formatted,sizeof(formatted)) == formatted);

	RETURN_STATUS;
}

/**
 * @brief Check integer formatting and caller-provided buffer isolation
 *
 * @details Confirms _Generic routing for boolean and signed integer
 * types, validates platform-dependent extremes (INTMAX_MIN, INTMAX_MAX,
 * UINTMAX_MAX) by stripping commas and comparing against the standard
 * %PRIdMAX/%PRIuMAX text, verifies that consecutive calls into two
 * separate destination buffers leave the earlier buffer untouched, and
 * verifies that the integer backends return the caller-provided buffer
 * pointer for successful calls
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_2(void)
{
	INITTEST;

	char formatted[MAX_CHARACTERS];
	char first[FORM_OUTPUT_BUFFER_SIZE];
	char second[FORM_OUTPUT_BUFFER_SIZE];

	/* The generic form() macro must dispatch boolean and signed integer values to integer formatting */
	ASSERT(0 == strcmp(form((_Bool)0,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form((_Bool)1,formatted,sizeof(formatted)),"1"));
	ASSERT(0 == strcmp(form((int)-12345,formatted,sizeof(formatted)),"-12,345"));
	ASSERT(0 == strcmp(form((short)-12345,formatted,sizeof(formatted)),"-12,345"));

	/* Extremes are checked by stripping commas and comparing with the standard conversion */
	const char *intmin_formatted = form_intmax_r(INTMAX_MIN,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(INTMAX_MIN,intmin_formatted));
	const char *intmax_formatted = form_intmax_r(INTMAX_MAX,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(INTMAX_MAX,intmax_formatted));
	const char *intneg_formatted = form_intmax_r(-1234567,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(-1234567,intneg_formatted));
	const char *intpos_formatted = form_intmax_r(1234567,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(1234567,intpos_formatted));
	const char *uintmax_formatted = form_uintmax_r(UINTMAX_MAX,formatted,sizeof(formatted));
	ASSERT(form_uintmax_matches_standard(UINTMAX_MAX,uintmax_formatted));

	/* Two destination buffers must remain independent between consecutive calls */
	ASSERT(0 == strcmp(form((size_t)1234,first,sizeof(first)),"1,234"));
	ASSERT(0 == strcmp(form((size_t)5678,second,sizeof(second)),"5,678"));
	ASSERT(0 == strcmp(first,"1,234"));

	/* A successful call into either integer backend must return the caller-provided buffer pointer */
	ASSERT(form_intmax_r((intmax_t)42,formatted,sizeof(formatted)) == formatted);
	ASSERT(form_uintmax_r((uintmax_t)42U,formatted,sizeof(formatted)) == formatted);

	RETURN_STATUS;
}

/**
 * @brief Check numeric formatter behavior with tiny destination buffers
 *
 * @details Covers shrink-to-fit behavior of form_real_r() (silent
 * reduction of fractional precision until the result fits, or fallback
 * to an empty terminated string when even the integer part cannot be
 * written), the non-finite branch (NaN, +-Inf and values larger than
 * UINTMAX_MAX become empty strings), the all-or-nothing behavior of
 * the integer formatters (either the full grouped value fits or the
 * result is empty — no partial digits are ever returned), the extra
 * byte that the leading '-' demands for signed integers, the rule that
 * a successful shrink-to-fit result still returns the caller-provided
 * buffer pointer instead of a literal, and explicit NULL/size=0 input
 * validation for form_real_r(), form_intmax_r() and form_uintmax_r()
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_3(void)
{
	INITTEST;

	char formatted[MAX_CHARACTERS];
	char tiny_real_1[1] = {'X'};
	/* Two two-byte buffers are kept separate so the zero-value case and the
	   nonzero-value case do not share initial contents between assertions */
	char tiny_real_2[2] = {'X','Y'};
	char tiny_real_2_nonzero[2] = {'X','Y'};
	char tiny_real_zero_1[1] = {'X'};
	char tiny_real_group_fail[5] = {'X','X','X','X','X'};
	char tiny_real_group_ok[6] = {'X','X','X','X','X','X'};
	char tiny_real_frac_fail[7] = {'X','X','X','X','X','X','X'};
	char tiny_real_frac_ok[8] = {'X','X','X','X','X','X','X','X'};
	char tiny_uint_1[1] = {'X'};
	char tiny_uint_2[2] = {'X','Y'};
	char tiny_uint_3[3] = {'X','Y','Z'};
	char tiny_uint_comma_fail[5] = {'X','Y','Z','Q','W'};
	char tiny_uint_7[7] = {'X','Y','Z','Q','W','E','R'};
	char tiny_int_1[1] = {'X'};
	char tiny_int_2[2] = {'X','Y'};
	char tiny_int_3[3] = {'X','Y','Z'};
	char tiny_int_6[6] = {'X','Y','Z','Q','W','E'};
	char tiny_int_7[7] = {'X','Y','Z','Q','W','E','R'};

	/* NULL output buffer and zero output size must produce a literal empty string without writing through the pointer */
	ASSERT(0 == strcmp(form_real_r(1.0L,NULL,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r(1.0L,formatted,0U),""));
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)1,NULL,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)1,formatted,0U),""));
	ASSERT(0 == strcmp(form_uintmax_r((uintmax_t)1U,NULL,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_uintmax_r((uintmax_t)1U,formatted,0U),""));

	/* Real formatting must either fit, reduce precision, or write an empty terminated string.
	   A 1-byte buffer cannot even hold a terminator behind a digit, so both the zero
	   fast path and the regular path write only '\0'. A 2-byte buffer exercises two
	   different code paths in form_real_r() that both happen to produce a single-digit
	   result:
	   - 1.25 takes the regular path that calls shrink-to-fit until the fractional part
	     is fully dropped, leaving "1"
	   - 0.0 takes the dedicated form_write_zero() fast path, which requires
	     result_size > 1 to write "0" */
	(void)form_real_r(1234.5L,tiny_real_1,sizeof(tiny_real_1));
	ASSERT(tiny_real_1[0] == '\0');
	(void)form_real_r(0.0L,tiny_real_zero_1,sizeof(tiny_real_zero_1));
	ASSERT(tiny_real_zero_1[0] == '\0');
	ASSERT(0 == strcmp(form_real_r(1.25L,tiny_real_2_nonzero,sizeof(tiny_real_2_nonzero)),"1"));
	ASSERT(0 == strcmp(form_real_r(0.0L,tiny_real_2,sizeof(tiny_real_2)),"0"));

	/* The next four buffer sizes walk the boundary for "1,234" and "1,234.5":
	   - 5 bytes cannot fit "1,234" plus its terminator (6 bytes total), so the result must be empty
	   - 6 bytes fit "1,234" plus its terminator exactly
	   - 7 bytes still cannot fit "1,234.5" plus its terminator (8 bytes total), so the fractional part is dropped with rounding ("1,235")
	   - 8 bytes fit "1,234.5" plus its terminator exactly.
	   For every shrink-to-fit branch the function must still return the caller-provided
	   buffer pointer, not a literal "" — this guards against a silent fallthrough into
	   form_write_empty() when shrink-to-fit succeeds */
	(void)form_real_r(1234.0L,tiny_real_group_fail,sizeof(tiny_real_group_fail));
	ASSERT(tiny_real_group_fail[0] == '\0');

	const char *group_ok_result = form_real_r(1234.0L,tiny_real_group_ok,sizeof(tiny_real_group_ok));
	ASSERT(group_ok_result == tiny_real_group_ok);
	ASSERT(0 == strcmp(group_ok_result,"1,234"));

	const char *frac_fail_result = form_real_r(1234.5L,tiny_real_frac_fail,sizeof(tiny_real_frac_fail));
	ASSERT(frac_fail_result == tiny_real_frac_fail);
	ASSERT(0 == strcmp(frac_fail_result,"1,235"));

	const char *frac_ok_result = form_real_r(1234.5L,tiny_real_frac_ok,sizeof(tiny_real_frac_ok));
	ASSERT(frac_ok_result == tiny_real_frac_ok);
	ASSERT(0 == strcmp(frac_ok_result,"1,234.5"));

	/* Non-finite real values are intentionally represented as empty strings */
	ASSERT(0 == strcmp(form_real_r(NAN,formatted,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r(INFINITY,formatted,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r(-INFINITY,formatted,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r((long double)UINTMAX_MAX * 2.0L,formatted,sizeof(formatted)),""));

	/* Unsigned integer formatting has no truncation mode: either the full value with its
	   thousands separators fits, or the buffer is left as an empty NUL-terminated string.
	   The "X"/"Y"/"Z" sentinels in the destination buffers must not be left visible:
	   - 1 byte: holds only the terminator, even for UINTMAX_MAX
	   - 2 and 3 bytes: too small for "12,345" (which needs 7 bytes including the terminator), result must be empty
	   - 5 bytes: enough for the digits of "1234" but not for the comma in "1,234" (6 bytes), so empty
	   - 7 bytes: exactly fits "12,345" plus its terminator */
	(void)form_uintmax_r(UINTMAX_MAX,tiny_uint_1,sizeof(tiny_uint_1));
	ASSERT(tiny_uint_1[0] == '\0');
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_2,sizeof(tiny_uint_2)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_3,sizeof(tiny_uint_3)),""));
	ASSERT(0 == strcmp(form_uintmax_r(1234U,tiny_uint_comma_fail,sizeof(tiny_uint_comma_fail)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_7,sizeof(tiny_uint_7)),"12,345"));

	/* Signed integer formatting follows the same all-or-nothing rule, but the leading
	   '-' takes one extra byte and shifts every boundary by one:
	   - 1 byte: holds only the terminator
	   - 2 bytes: enough digits for "-1" by digit count, but no room for the sign byte, so empty
	   - 3 bytes: exactly fits "-1" with its terminator
	   - 6 bytes: enough for "1,234" but not for "-1,234" (7 bytes with the terminator), so empty
	   - 7 bytes: exactly fits "-1,234" with its terminator */
	(void)form_intmax_r((intmax_t)-1,tiny_int_1,sizeof(tiny_int_1));
	ASSERT(tiny_int_1[0] == '\0');
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)-1,tiny_int_2,sizeof(tiny_int_2)),""));
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)-1,tiny_int_3,sizeof(tiny_int_3)),"-1"));
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)-1234,tiny_int_6,sizeof(tiny_int_6)),""));
	ASSERT(0 == strcmp(form_intmax_r((intmax_t)-1234,tiny_int_7,sizeof(tiny_int_7)),"-1,234"));

	RETURN_STATUS;
}

/**
 * @brief Check test-visible private form helper guards
 *
 * @details Directly calls helpers that are normally static. These branches are
 * defensive guards behind the public form API and cannot all be reached through
 * the public entry points alone
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_6(void)
{
	INITTEST;

	char buffer[2] = {'X','Y'};

	/* form_write_empty() returns a literal for invalid storage and clears valid storage */
	ASSERT(0 == strcmp(form_write_empty(NULL,sizeof(buffer)),""));
	ASSERT(0 == strcmp(form_write_empty(buffer,0U),""));
	ASSERT(form_write_empty(buffer,sizeof(buffer)) == buffer);
	ASSERT(buffer[0] == '\0');
	ASSERT(buffer[1] == 'Y');

	/* form_write_zero() delegates invalid and too-small storage to form_write_empty() */
	buffer[0] = 'X';
	buffer[1] = 'Y';
	ASSERT(0 == strcmp(form_write_zero(NULL,sizeof(buffer)),""));
	ASSERT(0 == strcmp(form_write_zero(buffer,0U),""));
	ASSERT(buffer[0] == 'X');
	ASSERT(buffer[1] == 'Y');

	ASSERT(form_write_zero(buffer,1U) == buffer);
	ASSERT(buffer[0] == '\0');
	ASSERT(buffer[1] == 'Y');

	buffer[0] = 'X';
	buffer[1] = 'Y';
	ASSERT(form_write_zero(buffer,sizeof(buffer)) == buffer);
	ASSERT(0 == strcmp(buffer,"0"));

	RETURN_STATUS;
}

/**
 * @brief Check byte-size formatting in static and caller-provided buffers
 *
 * @details Validates both the static-buffer variant bkbmbgbtbpbeb()
 * (FULL_VIEW for the full decomposition, MAJOR_VIEW for the largest
 * unit only with floor semantics — a 1.2 GiB input must produce "1GiB",
 * never "2GiB") and the reentrant variant bkbmbgbtbpbeb_r() (NULL/size=0
 * rejection, caller-buffer isolation between consecutive calls, return
 * of the caller-provided buffer pointer for successful calls, and
 * well-terminated truncation when the destination buffer is too small
 * to hold the full decomposition). The simulated snprintf() failure
 * branch is also exercised through the testmocking helpers
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_4(void)
{
	INITTEST;

	char bkb_r_a[MAX_CHARACTERS];
	char bkb_r_b[MAX_CHARACTERS];
	char bkb_r_tiny_1[1] = {'X'};
	char bkb_r_tiny_2[2] = {'X','Y'};
	char bkb_r_tiny_3[3] = {'X','Y','Z'};
	char bkb_r_tiny_6[6] = {'X','X','X','X','X','X'};
	char bkb_r_tiny_9[9] = {'X','X','X','X','X','X','X','X','X'};
	char bkb_r_tiny_10[10] = {'X','X','X','X','X','X','X','X','X','X'};
	char bkb_r_snprintf_failure[MAX_CHARACTERS] = "not empty";
	const size_t kibibyte = 1024ULL;
	const size_t mebibyte = kibibyte * 1024ULL;
	const size_t gibibyte = mebibyte * 1024ULL;
	const size_t tebibyte = gibibyte * 1024ULL;
	const size_t pebibyte = tebibyte * 1024ULL;
	const size_t exbibyte = pebibyte * 1024ULL;
	const size_t mixed_units = 4ULL * exbibyte
	                         + 5ULL * pebibyte
	                         + 6ULL * tebibyte
	                         + 7ULL * gibibyte
	                         + 8ULL * mebibyte
	                         + 9ULL * kibibyte
	                         + 10ULL;

	/* Static-buffer formatting must support full output and largest-unit-only output.
	   The last assertion uses 1,291,845,632 bytes = 1 GiB + 208 MiB; in MAJOR_VIEW the
	   formatter must show only the largest unit ("1GiB") and never round upward despite
	   the sizable remainder, otherwise a 1.5 GiB value would mislead callers as "2GiB" */
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,FULL_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1024,FULL_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,FULL_VIEW),"1KiB 512B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,MAJOR_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,MAJOR_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1291845632ULL,MAJOR_VIEW),"1GiB"));

	/* Reentrant formatting must reject invalid output buffers before writing */
	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,NULL,sizeof(bkb_r_a)));
	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,bkb_r_a,0U));

	/* A successful reentrant call must return the caller-provided buffer pointer, not an internal static */
	ASSERT(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)) == bkb_r_a);

	/* Valid reentrant calls should mirror the static-buffer formatter */
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB 512B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1291845632ULL,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1GiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(kibibyte - 1ULL,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1023B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(mebibyte - 1ULL,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1023KiB 1023B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(mebibyte,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1MiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(gibibyte,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1GiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(tebibyte,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1TiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(pebibyte,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1PiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(exbibyte,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1EiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(tebibyte,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1TiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(exbibyte,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1EiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(exbibyte - 1ULL,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1023PiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(mixed_units,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"4EiB 5PiB 6TiB 7GiB 8MiB 9KiB 10B"));

	/* Writing into a second caller buffer must not modify the first one */
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(2048U,FULL_VIEW,bkb_r_b,sizeof(bkb_r_b)),"2KiB"));
	ASSERT(0 == strcmp(bkb_r_a,"1KiB"));

	/* Tiny buffers are allowed to hold truncated but still terminated output */
	(void)bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_1,sizeof(bkb_r_tiny_1));
	ASSERT('\0' == bkb_r_tiny_1[0]);
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_2,sizeof(bkb_r_tiny_2)),"0"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_3,sizeof(bkb_r_tiny_3)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_6,sizeof(bkb_r_tiny_6)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_9,sizeof(bkb_r_tiny_9)),"1KiB 512"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_10,sizeof(bkb_r_tiny_10)),"1KiB 512B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(mixed_units,FULL_VIEW,bkb_r_tiny_10,sizeof(bkb_r_tiny_10)),"4EiB 5PiB"));

	/* Simulated snprintf() failure must leave the caller with an empty, terminated result */
	testmocking_snprintf_fail_next(1);
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(kibibyte,FULL_VIEW,bkb_r_snprintf_failure,sizeof(bkb_r_snprintf_failure)),""));
	testmocking_snprintf_disable();

	RETURN_STATUS;
}

/**
 * @brief Check duration formatting in static and caller-provided buffers
 *
 * @details Exercises form_date() and form_date_r() over nanosecond inputs.
 * Confirms zero handling, mixed-unit FULL_VIEW output, single-unit
 * MAJOR_VIEW output across every supported unit from nanoseconds through
 * years, successful calls returning their caller-provided buffer pointer,
 * buffer isolation, and terminated truncation in tiny buffers down to a
 * single byte. Link-time snprintf mock coverage for form_date_r() lives
 * separately in the test_librational_0003 suite to keep this test runnable
 * on platforms that lack GNU ld --wrap
 *
 * @return Return describing success or failure
 */
static Return test_librational_0001_5(void)
{
	INITTEST;

	char date_r_a[MAX_CHARACTERS];
	char date_r_b[MAX_CHARACTERS];
	char date_r_tiny_1[1] = {'X'};
	char date_r_tiny_2[2] = {'X','Y'};
	char date_r_tiny_3[3] = {'X','Y','Z'};
	char date_r_tiny_truncated_unit[4] = {'X','X','X','X'};
	const long long int ns_in_microsecond = 1000LL;
	const long long int ns_in_millisecond = 1000000LL;
	const long long int ns_in_second = 1000000000LL;
	const long long int ns_in_minute = 60000000000LL;
	const long long int ns_in_hour = 3600000000000LL;
	const long long int ns_in_day = 86400000000000LL;
	const long long int ns_in_week = 604800000000000LL;
	const long long int ns_in_month = 2628000000000000LL;
	const long long int ns_in_year = 31536000000000000LL;

	/* Static-buffer formatting must support full output and largest-unit-only output */
	ASSERT(0 == strcmp(form_date(0LL,FULL_VIEW),"0ns"));
	ASSERT(0 == strcmp(form_date(0LL,MAJOR_VIEW),"0ns"));
	ASSERT(0 == strcmp(form_date(273000528LL,FULL_VIEW),"273ms 528ns"));
	ASSERT(0 == strcmp(form_date(273000528LL,MAJOR_VIEW),"273ms"));
	ASSERT(0 == strcmp(form_date(3600000000001LL,FULL_VIEW),"1h 1ns"));
	ASSERT(0 == strcmp(form_date(3600000000001LL,MAJOR_VIEW),"1h"));

	/* A successful reentrant call must return the caller-provided buffer pointer, not an internal static */
	ASSERT(form_date_r(1LL,FULL_VIEW,date_r_a,sizeof(date_r_a)) == date_r_a);

	/* Valid and compact reentrant buffers should receive terminated output */
	ASSERT(0 == strcmp(form_date_r(273000528LL,FULL_VIEW,date_r_a,sizeof(date_r_a)),"273ms 528ns"));
	ASSERT(0 == strcmp(form_date_r(273000528LL,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"273ms"));
	ASSERT(0 == strcmp(form_date_r(ns_in_microsecond - 1LL,FULL_VIEW,date_r_a,sizeof(date_r_a)),"999ns"));
	ASSERT(0 == strcmp(form_date_r(ns_in_microsecond,FULL_VIEW,date_r_a,sizeof(date_r_a)),"1μs"));
	ASSERT(0 == strcmp(form_date_r(ns_in_millisecond - 1LL,FULL_VIEW,date_r_a,sizeof(date_r_a)),"999μs 999ns"));
	ASSERT(0 == strcmp(form_date_r(ns_in_millisecond,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1ms"));
	ASSERT(0 == strcmp(form_date_r(ns_in_second - 1LL,FULL_VIEW,date_r_a,sizeof(date_r_a)),"999ms 999μs 999ns"));
	ASSERT(0 == strcmp(form_date_r(ns_in_second,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1s"));
	ASSERT(0 == strcmp(form_date_r(ns_in_minute,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1min"));
	ASSERT(0 == strcmp(form_date_r(ns_in_hour,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1h"));
	ASSERT(0 == strcmp(form_date_r(ns_in_day,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1d"));
	ASSERT(0 == strcmp(form_date_r(ns_in_week,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1w"));
	ASSERT(0 == strcmp(form_date_r(ns_in_month,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1mon"));
	ASSERT(0 == strcmp(form_date_r(ns_in_year,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"1y"));

	/* Writing into a second caller buffer must not modify the first one.
	   Two values are picked from different unit branches so an accidental shared
	   static buffer would visibly leak the latter value into the former */
	ASSERT(0 == strcmp(form_date_r(ns_in_second,FULL_VIEW,date_r_a,sizeof(date_r_a)),"1s"));
	ASSERT(0 == strcmp(form_date_r(ns_in_minute,FULL_VIEW,date_r_b,sizeof(date_r_b)),"1min"));
	ASSERT(0 == strcmp(date_r_a,"1s"));

	/* The next group walks tiny destination buffers for form_date_r():
	   - 1 byte holds only the terminator, so any nonzero duration yields an empty string
	   - 2 bytes fit the literal "0" for the zero-time fast path, but not the trailing "ns" unit
	   - 3 bytes fit "0n" — zero plus a truncated unit suffix, still NUL terminated
	   - 4 bytes fit "1y" plus its terminator for the largest unit, and any further units
	     produced by FULL_VIEW past that point are dropped because catdate_r() refuses
	     to write into a buffer that is already full */
	(void)form_date_r(273000528LL,FULL_VIEW,date_r_tiny_1,sizeof(date_r_tiny_1));
	ASSERT('\0' == date_r_tiny_1[0]);
	ASSERT(0 == strcmp(form_date_r(0LL,FULL_VIEW,date_r_tiny_2,sizeof(date_r_tiny_2)),"0"));
	ASSERT(0 == strcmp(form_date_r(0LL,FULL_VIEW,date_r_tiny_3,sizeof(date_r_tiny_3)),"0n"));
	ASSERT(0 == strcmp(form_date_r(ns_in_year + 1LL,FULL_VIEW,date_r_tiny_truncated_unit,sizeof(date_r_tiny_truncated_unit)),"1y"));

	RETURN_STATUS;
}

/**
 * @brief Run librational formatting helper tests
 *
 * The suite covers numeric formatting, byte-size formatting, and duration
 * formatting. It checks exact stable text where practical, portable numeric
 * equivalence for long double cases, invalid argument handling, tiny buffer
 * behavior, and caller-provided buffer isolation
 *
 * Covered API surface:
 * - generic dispatch macro form() and its concrete backends
 *   form_real_r(), form_intmax_r() and form_uintmax_r()
 * - byte-size formatters bkbmbgbtbpbeb() and bkbmbgbtbpbeb_r()
 * - duration formatters form_date() and form_date_r()
 *
 * @return Return describing success or failure
 */
Return test_librational_0001(void)
{
	INITTEST;

	TEST(test_librational_0001_1,"form() formats real values with grouping, rounding, _Generic dispatch and caller-buffer return…");
	TEST(test_librational_0001_2,"integer formatters add grouping, cover platform extremes and preserve caller buffers…");
	TEST(test_librational_0001_3,"numeric formatters validate NULL/size=0, non-finite values and tiny destination buffers…");
	TEST(test_librational_0001_6,"private form helpers guard invalid storage and tiny zero buffers…");
	TEST(test_librational_0001_4,"byte-size formatters cover full and major views, tiny buffers and snprintf failure…");
	TEST(test_librational_0001_5,"duration formatters cover full and major views, two-buffer isolation and tiny buffers…");

	RETURN_STATUS;
}
