#include "rational.h"
#include <errno.h>

// A utility function to reverse a string
static void reverse(
	char   str[],
	size_t length)
{
	size_t start = 0;
	size_t end = 0;

	// Guard against empty strings
	if(length > 0)
	{
		end = length - 1;
	}

	while(start < end)
	{
		char temp = str[start];
		str[start] = str[end];
		str[end] = temp;
		end--;
		start++;
	}
}

/**
 * @brief Converts an integer to a string representation
 *
 * @param value The integer value to convert
 * @param str The destination string buffer
 * @param base The base for conversion (2-36)
 * @return char* Pointer to the resulting string
 *
 * @note The buffer should be large enough to hold the result
 * @note Supports negative numbers only in base 10
 * @note For invalid base (not in 2-36), sets errno = EINVAL,
 *       writes an empty string, and returns str
 * @note If str is NULL, sets errno = EINVAL and returns NULL
 */
char *itoa(
	int          num,
	char         *str,
	unsigned int base)
{
	size_t i = 0;
	bool isNegative = false;
	unsigned int unum; // Use unsigned int for calculations

	if(str == NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	if(base < 2 || base > 36)
	{
		errno = EINVAL;
		str[0] = '\0';
		return str;
	}

	/* Handle 0 explicitly, otherwise empty string is
	 * printed for 0 */
	if(num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	/* Handle negative numbers */
	if(num < 0 && base == 10)
	{
		/* The number is negative and we are converting to base 10.
		 * In this case we want to print a leading '-' sign and work
		 * with the absolute value of the number. For other bases we
		 * will treat the value as unsigned and show its bit pattern.
		 */
		isNegative = true;

		/* IMPORTANT:
		 * We must NOT write: (unsigned int)(-num)
		 * because the expression (-num) is evaluated in signed int,
		 * which is undefined behavior when num == INT_MIN
		 * (e.g. -2147483648 on a 32-bit system), since +2147483648
		 * cannot be represented in a signed int.
		 *
		 * To avoid this, we first cast num to unsigned int, and only
		 * then apply the unary minus. The unary minus on an unsigned
		 * type is well-defined: it performs modular arithmetic
		 * (2^N - value), so it never overflows.
		 *
		 * Example for a 32-bit int:
		 *   num        = INT_MIN         = -2147483648
		 *   (unsigned)num = 0x80000000  (2147483648u)
		 *   -(unsigned)num = 0x80000000 (still 2147483648u)
		 * This is exactly the magnitude we need for INT_MIN.
		 */
		unum = (unsigned int)(-(unsigned int)num);
	} else {
		/* For non-negative numbers, or for any base other than 10,
		 * we simply convert the value to unsigned.
		 *
		 * - If num >= 0, this just gives the same numeric value.
		 * - If num < 0 and base != 10, the result is the unsigned
		 *   value modulo 2^N, which is useful for hexadecimal or
		 *   binary dumps of the underlying representation.
		 */
		unum = (unsigned int)num;
	}

	/* Process individual digits */
	while(unum != 0)
	{
		int rem = (int)(unum % base);
		str[i++] = (rem > 9) ? (char)(rem - 10 + 'A') : (char)(rem + '0');
		unum = unum / base;
	}

	/* If number is negative, append '-' */
	if(isNegative)
	{
		str[i++] = '-';
	}

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	reverse(str,i);

	return str;
}
