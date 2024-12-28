#include "rational.h"

// A utility function to reverse a string
static void reverse(
	char str[],
	size_t length
){
	size_t start = 0;
	size_t end = length > 0 ? length - 1 : 0;  // Guard against empty strings
	while (start < end) {
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
 */
char* itoa(int num, char* str, unsigned int base)
{
	size_t i = 0;
	bool isNegative = false;
	unsigned int unum; // Use unsigned int for calculations

	/* Handle 0 explicitly, otherwise empty string is
	* printed for 0 */
	if (num == 0) {
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	/* Handle negative numbers */
	if (num < 0 && base == 10) {
		isNegative = true;
		unum = (unsigned int)(-num); // Convert to unsigned after negation
	} else {
		unum = (unsigned int)num;
	}

	/* Process individual digits */
	while (unum != 0) {
		int rem = (int)(unum % base);
		str[i++] = (rem > 9) ? (char)(rem - 10 + 'A') : (char)(rem + '0');
		unum = unum / base;
	}

	/* If number is negative, append '-' */
	if (isNegative)
		str[i++] = '-';

	str[i] = '\0'; // Append string terminator

	// Reverse the string
	reverse(str, i);

	return str;
}

#if 0
#include <limits.h>
/// Test

static void test_conversion(
	int value,
	int base,
	char *string
){
	char buffer[66];  /* 64 bits + sign + null terminator */
	itoa(value, buffer, base);

	/* Print original value in decimal and result in specified base */
	printf("Value: %d (decimal)\n", value);
	printf("Base %2d result: %s, %s\n", base, buffer,string);
	printf("-------------------\n");
}

/**
 * @brief Test program for itoa function
 *
 * @note Tests edge cases and different bases with special focus on
 *       negative numbers and MIN/MAX integer values
 */
int main(void)
{

	/* Test extreme values */
	printf("=== Testing extreme values ===\n");
	test_conversion(INT_MAX, 10, "2147483647");
	test_conversion(INT_MIN, 10, "-2147483648");
	test_conversion(INT_MIN, 16, "Should show in hex");

	/* Test regular cases */
	printf("\n=== Testing regular values ===\n");
	test_conversion(255, 16, "FF");
	test_conversion(255, 2, "11111111");
	test_conversion(-255, 10, "-255");

	/* Test zero handling */
	printf("\n=== Testing zero ===\n");
	test_conversion(0, 10, "0");
	test_conversion(0, 16, "0");
	test_conversion(0, 2, "0");

	/* Test larger bases */
	printf("\n=== Testing different bases ===\n");
	test_conversion(12345, 36, "Maximum supported base");
	test_conversion(12345, 16, "Common hex value");
	test_conversion(12345, 8, "Octal");

	/* Test negative values in different bases */
	printf("\n=== Testing negative values ===\n");
	test_conversion(-12345, 10, "Only base 10 shows negative sign");
	test_conversion(-12345, 16, "Should show unsigned hex");
	test_conversion(-1, 2, "All bits set");

	printf("\n=== Few more examples ===\n");
	char buffer[33];  /* Buffer for 32-bit integer */

	/* Decimal conversion */
	itoa(12345, buffer, 10);
	puts(buffer);

	/* Hexadecimal conversion */
	itoa(255, buffer, 16);
	puts(buffer);

	/* Binary conversion */
	itoa(15, buffer, 2);
	puts(buffer);

	/* Negative number */
	itoa(-789, buffer, 10);
	puts(buffer);

	/* Zero case */
	itoa(0, buffer, 10);
	puts(buffer);

		char str[100];
	printf("Number: %d\nBase: %d\tConverted String: %s\n",
		1567, 10, itoa(1567, str, 10));
	printf("Base: %d\t\tConverted String: %s\n", 2,
		itoa(1567, str, 2));
	printf("Base: %d\t\tConverted String: %s\n", 8,
		itoa(1567, str, 8));
	printf("Base: %d\tConverted String: %s\n", 16,
		itoa(1567, str, 16));

	return 0; // Success
}
#endif
