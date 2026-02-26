#include "sute.h"

#include <float.h>

static void strip_commas(
	const char *src,
	char       *dst,
	size_t      dst_size)
{
	if(dst == NULL || dst_size == 0U)
	{
		return;
	}

	if(src == NULL)
	{
		dst[0] = '\0';
		return;
	}

	size_t write = 0U;
	for(size_t read = 0U; src[read] != '\0'; read++)
	{
		if(src[read] == ',')
		{
			continue;
		}

		if(write + 1U >= dst_size)
		{
			dst[0] = '\0';
			return;
		}

		dst[write++] = src[read];
	}

	dst[write] = '\0';
}

static bool valid_comma_grouping(
	const char *val)
{
	if(val == NULL || val[0] == '\0')
	{
		return false;
	}

	size_t start = 0U;
	if(val[0] == '-')
	{
		start = 1U;
	}

	size_t len = strlen(val);
	if(len <= start)
	{
		return false;
	}

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

static bool form_intmax_matches_standard(
	intmax_t    val,
	const char *formatted)
{
	char expected[MAX_CHARACTERS];
	char stripped[MAX_CHARACTERS];

	if(snprintf(expected,sizeof(expected),"%" PRIdMAX,val) < 0)
	{
		return false;
	}

	strip_commas(formatted,stripped,sizeof(stripped));

	return valid_comma_grouping(formatted) && (0 == strcmp(stripped,expected));
}

static bool form_uintmax_matches_standard(
	uintmax_t   val,
	const char *formatted)
{
	char expected[MAX_CHARACTERS];
	char stripped[MAX_CHARACTERS];

	if(snprintf(expected,sizeof(expected),"%" PRIuMAX,val) < 0)
	{
		return false;
	}

	strip_commas(formatted,stripped,sizeof(stripped));

	return valid_comma_grouping(formatted) && (0 == strcmp(stripped,expected));
}

static bool valid_grouped_integer_part(
	const char *val,
	size_t     start,
	size_t     end)
{
	if(val == NULL || end <= start)
	{
		return false;
	}

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

static bool valid_grouped_real(
	const char *val)
{
	if(val == NULL || val[0] == '\0')
	{
		return false;
	}

	size_t start = 0U;
	if(val[0] == '-')
	{
		start = 1U;
	}

	if(val[start] == '\0')
	{
		return false;
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

static bool form_real_matches_portable(
	long double expected,
	const char  *formatted)
{
	if(valid_grouped_real(formatted) == false)
	{
		return false;
	}

	char stripped[MAX_CHARACTERS];
	strip_commas(formatted,stripped,sizeof(stripped));

	errno = 0;
	char *end = NULL;
	const long double parsed = strtold(stripped,&end);
	if(errno != 0 || end == stripped || *end != '\0')
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

static Return test0032_1(void)
{
	INITTEST;

	// Shared destination for most formatter calls in this test.
	// We pass this buffer into form()/form_*_r() and compare the produced text.
	char formatted[MAX_CHARACTERS];
	// Separate destinations to verify independent writes between two calls.
	char first[FORM_OUTPUT_BUFFER_SIZE];
	char second[FORM_OUTPUT_BUFFER_SIZE];

	// Intentionally tiny buffers prefilled with sentinels.
	// They are used to validate truncation and NUL-termination behavior.
	char tiny_real_1[1] = {'X'};
	char tiny_real_2[2] = {'X','Y'};
	char tiny_real_2_nonzero[2] = {'X','Y'};
	char tiny_real_group_fail[5] = {'X','X','X','X','X'};
	char tiny_real_group_ok[6] = {'X','X','X','X','X','X'};
	char tiny_real_frac_fail[7] = {'X','X','X','X','X','X','X'};
	char tiny_real_frac_ok[8] = {'X','X','X','X','X','X','X','X'};
	char tiny_uint_1[1] = {'X'};
	char tiny_uint_2[2] = {'X','Y'};
	char tiny_uint_3[3] = {'X','Y','Z'};
	char tiny_uint_7[7] = {'X','Y','Z','Q','W','E','R'};
	char bkb_r_a[MAX_CHARACTERS];
	char bkb_r_b[MAX_CHARACTERS];
	char bkb_r_tiny_1[1] = {'X'};
	char bkb_r_tiny_2[2] = {'X','Y'};
	char bkb_r_tiny_3[3] = {'X','Y','Z'};
	char bkb_r_tiny_6[6] = {'X','X','X','X','X','X'};
	char bkb_r_tiny_9[9] = {'X','X','X','X','X','X','X','X','X'};
	char bkb_r_tiny_10[10] = {'X','X','X','X','X','X','X','X','X','X'};
	char date_r_a[MAX_CHARACTERS];
	char date_r_tiny_1[1] = {'X'};
	char date_r_tiny_2[2] = {'X','Y'};
	char date_r_tiny_3[3] = {'X','Y','Z'};

	// Series 1: generic form() formatting into caller-provided buffer `formatted`.
	// Pattern: call form(value,formatted,sizeof(formatted)), then compare with exact text.
	ASSERT(0 == strcmp(form(0.0L,formatted,sizeof(formatted)),"0"));
	// Same path, near-zero negative rounds to textual zero.
	ASSERT(0 == strcmp(form(-0.0000000004L,formatted,sizeof(formatted)),"0"));
	// Same path, rounding at 9th decimal place.
	ASSERT(0 == strcmp(form(0.0000000006L,formatted,sizeof(formatted)),"0.000000001"));
	// Same behavior for negative rounding.
	ASSERT(0 == strcmp(form(-0.0000000006L,formatted,sizeof(formatted)),"-0.000000001"));
	// Grouping separators in integer part.
	ASSERT(0 == strcmp(form(1234567.0L,formatted,sizeof(formatted)),"1,234,567"));
	ASSERT(0 == strcmp(form(1234567890123456.0L,formatted,sizeof(formatted)),"1,234,567,890,123,456"));
	ASSERT(0 == strcmp(form(1234567.125L,formatted,sizeof(formatted)),"1,234,567.125"));
	ASSERT(0 == strcmp(form(-9876.5L,formatted,sizeof(formatted)),"-9,876.5"));
	// For long double cases, compare parsed numeric value with tolerance.
	// The formatted string still comes from the same `formatted` buffer.
	ASSERT(form_real_matches_portable(123456789.123456780L,form(123456789.123456780L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(987654321098.123456789L,form(987654321098.123456789L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(1.23L,form(1.23L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(9.9999999995L,form(9.9999999995L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(-9.9999999995L,form(-9.9999999995L,formatted,sizeof(formatted))));
	ASSERT(form_real_matches_portable(999.9999999995L,form(999.9999999995L,formatted,sizeof(formatted))));

	// Series 2: boolean/integer overloads still write to `formatted`.
	ASSERT(0 == strcmp(form((_Bool)0,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form((_Bool)1,formatted,sizeof(formatted)),"1"));
	ASSERT(0 == strcmp(form((int)-12345,formatted,sizeof(formatted)),"-12,345"));
	ASSERT(0 == strcmp(form((short)-12345,formatted,sizeof(formatted)),"-12,345"));

	// Series 3: form_intmax_r writes to `formatted`, then helper validates comma placement
	// and numeric equivalence against standard conversion for the same input value.
	const char *intmin_formatted = form_intmax_r(INTMAX_MIN,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(INTMAX_MIN,intmin_formatted));
	// Same comparison strategy for other intmax values.
	const char *intmax_formatted = form_intmax_r(INTMAX_MAX,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(INTMAX_MAX,intmax_formatted));
	const char *intneg_formatted = form_intmax_r(-1234567,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(-1234567,intneg_formatted));
	const char *intpos_formatted = form_intmax_r(1234567,formatted,sizeof(formatted));
	ASSERT(form_intmax_matches_standard(1234567,intpos_formatted));

	// Same idea for unsigned max conversion.
	const char *uintmax_formatted = form_uintmax_r(UINTMAX_MAX,formatted,sizeof(formatted));
	ASSERT(form_uintmax_matches_standard(UINTMAX_MAX,uintmax_formatted));

	// Series 4: independent output buffers (`first`, `second`) for two calls.
	// Then ensure `first` keeps its value after writing into `second`.
	ASSERT(0 == strcmp(form((size_t)1234,first,sizeof(first)),"1,234"));
	ASSERT(0 == strcmp(form((size_t)5678,second,sizeof(second)),"5,678"));
	ASSERT(0 == strcmp(first,"1,234"));

	// Series 5: tiny-buffer behavior for form_real_r.
	// Buffer too small => empty string.
	(void)form_real_r(1234.5L,tiny_real_1,sizeof(tiny_real_1));
	ASSERT(tiny_real_1[0] == '\0');
	// Still using tiny destinations, now checking reduced output variants.
	ASSERT(0 == strcmp(form_real_r(1.25L,tiny_real_2_nonzero,sizeof(tiny_real_2_nonzero)),"1"));
	ASSERT(0 == strcmp(form_real_r(0.0L,tiny_real_2,sizeof(tiny_real_2)),"0"));
	(void)form_real_r(1234.0L,tiny_real_group_fail,sizeof(tiny_real_group_fail));
	ASSERT(tiny_real_group_fail[0] == '\0');
	ASSERT(0 == strcmp(form_real_r(1234.0L,tiny_real_group_ok,sizeof(tiny_real_group_ok)),"1,234"));
	ASSERT(0 == strcmp(form_real_r(1234.5L,tiny_real_frac_fail,sizeof(tiny_real_frac_fail)),"1,235"));
	ASSERT(0 == strcmp(form_real_r(1234.5L,tiny_real_frac_ok,sizeof(tiny_real_frac_ok)),"1,234.5"));
	ASSERT(0 == strcmp(form_real_r(NAN,formatted,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r(INFINITY,formatted,sizeof(formatted)),""));
	ASSERT(0 == strcmp(form_real_r(-INFINITY,formatted,sizeof(formatted)),""));

	// Series 6: tiny-buffer behavior for form_uintmax_r (same pattern).
	(void)form_uintmax_r(UINTMAX_MAX,tiny_uint_1,sizeof(tiny_uint_1));
	ASSERT(tiny_uint_1[0] == '\0');
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_2,sizeof(tiny_uint_2)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_3,sizeof(tiny_uint_3)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_7,sizeof(tiny_uint_7)),"12,345"));

	// Series 7: bkbmbgbtbpbeb returns static text; compare FULL_VIEW outputs directly.
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,FULL_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1024,FULL_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,FULL_VIEW),"1KiB 512B"));

	// Same for MAJOR_VIEW (only the largest non-zero unit).
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,MAJOR_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,MAJOR_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1291845632ULL,MAJOR_VIEW),"1GiB"));

	// Series 8: bkbmbgbtbpbeb_r writes into caller buffer (`bkb_r_a`), check invalid args first.
	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,NULL,sizeof(bkb_r_a)));
	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,bkb_r_a,0U));

	// Valid writes into the same destination buffer, then strcmp with expected literals.
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB 512B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1291845632ULL,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1GiB"));

	// Cross-buffer isolation check: writing into bkb_r_b must not modify bkb_r_a.
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(2048U,FULL_VIEW,bkb_r_b,sizeof(bkb_r_b)),"2KiB"));
	ASSERT(0 == strcmp(bkb_r_a,"1KiB"));

	// Tiny destination checks for bkbmbgbtbpbeb_r.
	(void)bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_1,sizeof(bkb_r_tiny_1));
	ASSERT('\0' == bkb_r_tiny_1[0]);
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_2,sizeof(bkb_r_tiny_2)),"0"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_3,sizeof(bkb_r_tiny_3)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_6,sizeof(bkb_r_tiny_6)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_9,sizeof(bkb_r_tiny_9)),"1KiB 512"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_10,sizeof(bkb_r_tiny_10)),"1KiB 512B"));

	// Series 9: form_date (static output) in FULL_VIEW and MAJOR_VIEW.
	ASSERT(0 == strcmp(form_date(0LL,FULL_VIEW),"0ns"));
	ASSERT(0 == strcmp(form_date(0LL,MAJOR_VIEW),"0ns"));
	ASSERT(0 == strcmp(form_date(273000528LL,FULL_VIEW),"273ms 528ns"));
	ASSERT(0 == strcmp(form_date(273000528LL,MAJOR_VIEW),"273ms"));
	ASSERT(0 == strcmp(form_date(3600000000001LL,FULL_VIEW),"1h 1ns"));
	ASSERT(0 == strcmp(form_date(3600000000001LL,MAJOR_VIEW),"1h"));

	// form_date_r with explicit destination buffer and invalid-argument checks.
	ASSERT(NULL == form_date_r(1LL,FULL_VIEW,NULL,sizeof(date_r_a)));
	ASSERT(NULL == form_date_r(1LL,FULL_VIEW,date_r_a,0U));
	// Then normal writes and compact-buffer edge cases.
	ASSERT(0 == strcmp(form_date_r(273000528LL,FULL_VIEW,date_r_a,sizeof(date_r_a)),"273ms 528ns"));
	ASSERT(0 == strcmp(form_date_r(273000528LL,MAJOR_VIEW,date_r_a,sizeof(date_r_a)),"273ms"));
	(void)form_date_r(273000528LL,FULL_VIEW,date_r_tiny_1,sizeof(date_r_tiny_1));
	ASSERT('\0' == date_r_tiny_1[0]);
	ASSERT(0 == strcmp(form_date_r(0LL,FULL_VIEW,date_r_tiny_2,sizeof(date_r_tiny_2)),"0"));
	ASSERT(0 == strcmp(form_date_r(0LL,FULL_VIEW,date_r_tiny_3,sizeof(date_r_tiny_3)),"0n"));

	RETURN_STATUS;
}

Return test0032(void)
{
	INITTEST;

	TEST(test0032_1,"rational formatting helpers…");

	RETURN_STATUS;
}
