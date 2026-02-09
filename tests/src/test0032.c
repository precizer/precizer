#include "sute.h"

#include <inttypes.h>

#define SHOW_TEST 0

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

static Return test0032_formatters(void)
{
	INITTEST;

	char formatted[MAX_CHARACTERS];
	char first[FORM_OUTPUT_BUFFER_SIZE];
	char second[FORM_OUTPUT_BUFFER_SIZE];
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

	ASSERT(0 == strcmp(form(0.0L,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form(-0.0000000004L,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form(0.0000000006L,formatted,sizeof(formatted)),"0.000000001"));
	ASSERT(0 == strcmp(form(-0.0000000006L,formatted,sizeof(formatted)),"-0.000000001"));
	ASSERT(0 == strcmp(form(1234567.0L,formatted,sizeof(formatted)),"1,234,567"));
	ASSERT(0 == strcmp(form(1234567890123456.0L,formatted,sizeof(formatted)),"1,234,567,890,123,456"));
	ASSERT(0 == strcmp(form(1234567.125L,formatted,sizeof(formatted)),"1,234,567.125"));
	ASSERT(0 == strcmp(form(-9876.5L,formatted,sizeof(formatted)),"-9,876.5"));
	ASSERT(0 == strcmp(form(123456789.123456780L,formatted,sizeof(formatted)),"123,456,789.12345678"));
	ASSERT(0 == strcmp(form(987654321098.123456789L,formatted,sizeof(formatted)),"987,654,321,098.123456776"));
	ASSERT(0 == strcmp(form(123456789.123456780L,formatted,sizeof(formatted)),"123,456,789.12345678"));
	ASSERT(0 == strcmp(form(1.23L,formatted,sizeof(formatted)),"1.23"));
	ASSERT(0 == strcmp(form(9.9999999995L,formatted,sizeof(formatted)),"10"));
	ASSERT(0 == strcmp(form(-9.9999999995L,formatted,sizeof(formatted)),"-10"));
	ASSERT(0 == strcmp(form(999.9999999995L,formatted,sizeof(formatted)),"1,000"));

	ASSERT(0 == strcmp(form((_Bool)0,formatted,sizeof(formatted)),"0"));
	ASSERT(0 == strcmp(form((_Bool)1,formatted,sizeof(formatted)),"1"));
	ASSERT(0 == strcmp(form((int)-12345,formatted,sizeof(formatted)),"-12,345"));
	ASSERT(0 == strcmp(form((short)-12345,formatted,sizeof(formatted)),"-12,345"));

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

	ASSERT(0 == strcmp(form((size_t)1234,first,sizeof(first)),"1,234"));
	ASSERT(0 == strcmp(form((size_t)5678,second,sizeof(second)),"5,678"));
	ASSERT(0 == strcmp(first,"1,234"));

	(void)form_real_r(1234.5L,tiny_real_1,sizeof(tiny_real_1));
	ASSERT(tiny_real_1[0] == '\0');
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
	ASSERT(0 == strcmp(form_real_r((long double)UINTMAX_MAX + 1.0L,formatted,sizeof(formatted)),""));

	(void)form_uintmax_r(UINTMAX_MAX,tiny_uint_1,sizeof(tiny_uint_1));
	ASSERT(tiny_uint_1[0] == '\0');
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_2,sizeof(tiny_uint_2)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_3,sizeof(tiny_uint_3)),""));
	ASSERT(0 == strcmp(form_uintmax_r(12345U,tiny_uint_7,sizeof(tiny_uint_7)),"12,345"));

	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,FULL_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1024,FULL_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,FULL_VIEW),"1KiB 512B"));

	ASSERT(0 == strcmp(bkbmbgbtbpbeb(0,MAJOR_VIEW),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1536,MAJOR_VIEW),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb(1291845632ULL,MAJOR_VIEW),"1GiB"));

	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,NULL,sizeof(bkb_r_a)));
	ASSERT(NULL == bkbmbgbtbpbeb_r(1U,FULL_VIEW,bkb_r_a,0U));

	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB 512B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1291845632ULL,MAJOR_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1GiB"));

	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1024U,FULL_VIEW,bkb_r_a,sizeof(bkb_r_a)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(2048U,FULL_VIEW,bkb_r_b,sizeof(bkb_r_b)),"2KiB"));
	ASSERT(0 == strcmp(bkb_r_a,"1KiB"));

	(void)bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_1,sizeof(bkb_r_tiny_1));
	ASSERT('\0' == bkb_r_tiny_1[0]);
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_2,sizeof(bkb_r_tiny_2)),"0"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(0U,FULL_VIEW,bkb_r_tiny_3,sizeof(bkb_r_tiny_3)),"0B"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_6,sizeof(bkb_r_tiny_6)),"1KiB"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_9,sizeof(bkb_r_tiny_9)),"1KiB 512"));
	ASSERT(0 == strcmp(bkbmbgbtbpbeb_r(1536U,FULL_VIEW,bkb_r_tiny_10,sizeof(bkb_r_tiny_10)),"1KiB 512B"));

	RETURN_STATUS;
}

Return test0032(void)
{
	INITTEST;

	TEST(test0032_formatters,"rational formatting helpers…");

	RETURN_STATUS;
}
