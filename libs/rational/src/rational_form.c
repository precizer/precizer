#include "rational.h"

static const char *form_write_empty(
	char   *result,
	size_t result_size)
{
	if(result == NULL || result_size == 0U)
	{
		return "";
	}

	result[0] = '\0';
	return result;
}

static const char *form_write_zero(
	char   *result,
	size_t result_size)
{
	if(result == NULL || result_size == 0U)
	{
		return form_write_empty(result,result_size);
	}

	if(result_size <= 1U)
	{
		return form_write_empty(result,result_size);
	}

	result[0] = '0';
	result[1] = '\0';
	return result;
}

static const char *form_uintmax_with_sign_r(
	uintmax_t val,
	bool      negative,
	char      *result,
	size_t    result_size)
{
	if(result == NULL || result_size == 0U)
	{
		return "";
	}

	char *write = result + result_size - 1U;
	*write = '\0';
	size_t group = 0U;

	do
	{
		if(write == result)
		{
			return form_write_empty(result,result_size);
		}

		if(group == 3U)
		{
			*--write = ',';
			group = 0U;

			if(write == result)
			{
				return form_write_empty(result,result_size);
			}
		}

		const uintmax_t digit = val % 10U;
		*--write = (char)('0' + (int)digit);
		val /= 10U;
		group++;
	} while(val > 0U);

	if(negative)
	{
		if(write == result)
		{
			return form_write_empty(result,result_size);
		}

		*--write = '-';
	}

	if(write != result)
	{
		const size_t used = (size_t)((result + result_size) - write);
		memmove(result,write,used);
	}

	return result;
}

static size_t form_grouped_uintmax_len(uintmax_t val)
{
	size_t digits = 1U;

	while(val >= 10U)
	{
		val /= 10U;
		digits++;
	}

	const size_t groups = (digits - 1U) / 3U;
	return digits + groups;
}

/**
 *
 * @brief Format 1234567.89 -> 1,234,567.89
 * @details va_arg format got from https://stackoverflow.com/a/23647983/7104681
 * and https://stackoverflow.com/questions/1449805/how-to-format-a-number-using-comma-as-thousands-separator-in-c
 * Fractional precision is reduced silently when needed to make the formatted value fit into `result_size`.
 * @param val - Any long double digit
 * @return Pointer to a string
 *
 */
const char *form_real_r(
	long double val,
	char        *result,
	size_t      result_size)
{
	if(result == NULL || result_size == 0U)
	{
		return "";
	}

	const long double magnitude = fabsl(val);

	if(magnitude < 0.0000000001L)
	{
		return form_write_zero(result,result_size);
	}

	if(!isfinite(magnitude) || magnitude > (long double)UINTMAX_MAX)
	{
		return form_write_empty(result,result_size);
	}

	const uintmax_t integer_part = (uintmax_t)magnitude;
	long double fraction = magnitude - (long double)integer_part;
	unsigned char fraction_digits[10] = {0U};

	for(size_t i = 0U; i < 10U; i++)
	{
		fraction *= 10.0L;
		int digit = (int)fraction;

		if(digit < 0)
		{
			digit = 0;
		} else if(digit > 9){
			digit = 9;
		}

		fraction_digits[i] = (unsigned char)digit;
		fraction -= (long double)digit;
	}

	const bool negative = signbit(val) != 0;

	/* Reduce fractional precision only when needed to make the value fit. */
	for(size_t precision = 9U;; precision--)
	{
		uintmax_t rounded_integer_part = integer_part;
		unsigned char rounded_fraction_digits[9] = {0U};

		for(size_t i = 0U; i < precision; i++)
		{
			rounded_fraction_digits[i] = fraction_digits[i];
		}

		bool carry = fraction_digits[precision] >= 5U;

		for(size_t i = precision; i > 0U && carry; i--)
		{
			const size_t idx = i - 1U;
			const unsigned int rounded = (unsigned int)rounded_fraction_digits[idx] + 1U;
			rounded_fraction_digits[idx] = (unsigned char)(rounded % 10U);
			carry = rounded == 10U;
		}

		if(carry)
		{
			if(rounded_integer_part == UINTMAX_MAX)
			{
				if(precision == 0U)
				{
					return form_write_empty(result,result_size);
				}

				continue;
			}

			rounded_integer_part++;
		}

		size_t fractional_len = precision;

		while(fractional_len > 0U && rounded_fraction_digits[fractional_len - 1U] == 0U)
		{
			fractional_len--;
		}

		if(rounded_integer_part == 0U && fractional_len == 0U)
		{
			return form_write_zero(result,result_size);
		}

		const size_t integer_len = form_grouped_uintmax_len(rounded_integer_part);
		size_t required = integer_len + 1U;

		if(negative)
		{
			required++;
		}

		if(fractional_len > 0U)
		{
			required += 1U + fractional_len;
		}

		if(required > result_size)
		{
			if(precision == 0U)
			{
				return form_write_empty(result,result_size);
			}

			continue;
		}

		char *write = result + result_size - 1U;
		*write = '\0';

		if(fractional_len > 0U)
		{
			for(size_t i = fractional_len; i > 0U; i--)
			{
				*--write = (char)('0' + (int)rounded_fraction_digits[i - 1U]);
			}

			*--write = '.';
		}

		size_t group = 0U;
		uintmax_t integer_write_part = rounded_integer_part;

		do
		{
			if(group == 3U)
			{
				*--write = ',';
				group = 0U;
			}

			const uintmax_t digit = integer_write_part % 10U;
			*--write = (char)('0' + (int)digit);
			integer_write_part /= 10U;
			group++;
		} while(integer_write_part > 0U);

		if(negative)
		{
			*--write = '-';
		}

		if(write != result)
		{
			const size_t used = (size_t)((result + result_size) - write);
			memmove(result,write,used);
		}

		return result;
	}

	return form_write_empty(result,result_size);
}

const char *form_uintmax_r(
	uintmax_t val,
	char      *result,
	size_t    result_size)
{
	return form_uintmax_with_sign_r(val,false,result,result_size);
}

const char *form_intmax_r(
	intmax_t val,
	char     *result,
	size_t   result_size)
{
	const bool negative = val < 0;
	uintmax_t magnitude = 0U;

	if(negative)
	{
		magnitude = (uintmax_t)(-(val + 1)) + 1U;
	} else {
		magnitude = (uintmax_t)val;
	}

	return form_uintmax_with_sign_r(magnitude,negative,result,result_size);
}

// Example (manual smoke test)
#if 0
int main(void)
{
	char buf[FORM_OUTPUT_BUFFER_SIZE];
	long double digit = 837452834.94L;

	printf("digit: %s\n",form(digit,buf,sizeof(buf)));
	return 0;
}
#endif
