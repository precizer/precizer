#include "test_librational_all.h"
#include <errno.h>
#include <limits.h>

/* The expected stdout pattern is strict and covers the whole demonstration output.
 * Non-decimal negative output depends on the platform width of int, so
 * the test_librational_0002 suite uses a static assertion to keep these expectations
 * tied to the supported 32-bit int targets */
static const char expected_itoa_stdout_pattern[] =
        "\\A"
        "=== Testing extreme values ===\n"
        "Value: 2147483647 \\(decimal\\)\n"
        "Base 10 result: 2147483647, 2147483647\n"
        "-------------------\n"
        "Value: -2147483648 \\(decimal\\)\n"
        "Base 10 result: -2147483648, -2147483648\n"
        "-------------------\n"
        "Value: -2147483648 \\(decimal\\)\n"
        "Base 16 result: 80000000, Should show in hex\n"
        "-------------------\n"
        "\n"
        "=== Testing regular values ===\n"
        "Value: 255 \\(decimal\\)\n"
        "Base 16 result: FF, FF\n"
        "-------------------\n"
        "Value: 255 \\(decimal\\)\n"
        "Base  2 result: 11111111, 11111111\n"
        "-------------------\n"
        "Value: -255 \\(decimal\\)\n"
        "Base 10 result: -255, -255\n"
        "-------------------\n"
        "\n"
        "=== Testing zero ===\n"
        "Value: 0 \\(decimal\\)\n"
        "Base 10 result: 0, 0\n"
        "-------------------\n"
        "Value: 0 \\(decimal\\)\n"
        "Base 16 result: 0, 0\n"
        "-------------------\n"
        "Value: 0 \\(decimal\\)\n"
        "Base  2 result: 0, 0\n"
        "-------------------\n"
        "\n"
        "=== Testing different bases ===\n"
        "Value: 12345 \\(decimal\\)\n"
        "Base 36 result: 9IX, Maximum supported base\n"
        "-------------------\n"
        "Value: 12345 \\(decimal\\)\n"
        "Base 16 result: 3039, Common hex value\n"
        "-------------------\n"
        "Value: 12345 \\(decimal\\)\n"
        "Base  8 result: 30071, Octal\n"
        "-------------------\n"
        "\n"
        "=== Testing negative values ===\n"
        "Value: -12345 \\(decimal\\)\n"
        "Base 10 result: -12345, Only base 10 shows negative sign\n"
        "-------------------\n"
        "Value: -12345 \\(decimal\\)\n"
        "Base 16 result: FFFFCFC7, Should show unsigned hex\n"
        "-------------------\n"
        "Value: -1 \\(decimal\\)\n"
        "Base  2 result: 11111111111111111111111111111111, All bits set\n"
        "-------------------\n"
        "\n"
        "=== Few more examples ===\n"
        "12345\n"
        "FF\n"
        "1111\n"
        "-789\n"
        "0\n"
        "Number: 1567\n"
        "Base: 10\tConverted String: 1567\n"
        "Base: 2\t\tConverted String: 11000011111\n"
        "Base: 8\t\tConverted String: 3037\n"
        "Base: 16\tConverted String: 61F\n"
        "\n"
        "=== Invalid base handling ===\n"
        "Base 0 result: '', errno: 22\n"
        "Base 1 result: '', errno: 22\n"
        "Base 37 result: '', errno: 22\n"
        "\n"
        "=== NULL buffer handling ===\n"
        "NULL buffer result: NULL, errno: 22\\Z";

static void test_conversion(
	int          value,
	unsigned int base,
	const char   *reference_text)
{
	char buffer[66];  /* 64 bits + sign + null terminator */
	itoa(value,buffer,base);

	/* Print the converted value next to reference text for the strict stdout pattern */
	printf("Value: %d (decimal)\n",value);
	printf("Base %2u result: %s, %s\n",base,buffer,reference_text);
	printf("-------------------\n");
}

/**
 * @brief Test program for itoa function
 *
 * @note Tests edge cases and different bases with special focus on
 *       negative numbers and MIN/MAX integer values
 */
static void test_itoa(void)
{
	/* Test extreme values */
	printf("=== Testing extreme values ===\n");
	test_conversion(INT_MAX,10,"2147483647");
	test_conversion(INT_MIN,10,"-2147483648");
	test_conversion(INT_MIN,16,"Should show in hex");

	/* Test regular cases */
	printf("\n=== Testing regular values ===\n");
	test_conversion(255,16,"FF");
	test_conversion(255,2,"11111111");
	test_conversion(-255,10,"-255");

	/* Test zero handling */
	printf("\n=== Testing zero ===\n");
	test_conversion(0,10,"0");
	test_conversion(0,16,"0");
	test_conversion(0,2,"0");

	/* Test larger bases */
	printf("\n=== Testing different bases ===\n");
	test_conversion(12345,36,"Maximum supported base");
	test_conversion(12345,16,"Common hex value");
	test_conversion(12345,8,"Octal");

	/* Test negative values in different bases */
	printf("\n=== Testing negative values ===\n");
	test_conversion(-12345,10,"Only base 10 shows negative sign");
	test_conversion(-12345,16,"Should show unsigned hex");
	test_conversion(-1,2,"All bits set");

	printf("\n=== Few more examples ===\n");
	char buffer[33];  /* Buffer for 32-bit integer */

	/* Decimal conversion */
	itoa(12345,buffer,10);
	puts(buffer);

	/* Hexadecimal conversion */
	itoa(255,buffer,16);
	puts(buffer);

	/* Binary conversion */
	itoa(15,buffer,2);
	puts(buffer);

	/* Negative number */
	itoa(-789,buffer,10);
	puts(buffer);

	/* Zero case */
	itoa(0,buffer,10);
	puts(buffer);

	char str[100];
	printf("Number: %d\nBase: %d\tConverted String: %s\n",1567,10,itoa(1567,str,10));
	printf("Base: %d\t\tConverted String: %s\n",2,itoa(1567,str,2));
	printf("Base: %d\t\tConverted String: %s\n",8,itoa(1567,str,8));
	printf("Base: %d\tConverted String: %s\n",16,itoa(1567,str,16));

	printf("\n=== Invalid base handling ===\n");
	char errbuf[4];
	char *ret = NULL;

	errno = 0;
	ret = itoa(123,errbuf,0);
	printf("Base 0 result: '%s', errno: %d\n",ret ? ret : "NULL",errno);

	errno = 0;
	ret = itoa(123,errbuf,1);
	printf("Base 1 result: '%s', errno: %d\n",ret ? ret : "NULL",errno);

	errno = 0;
	ret = itoa(123,errbuf,37);
	printf("Base 37 result: '%s', errno: %d\n",ret ? ret : "NULL",errno);

	printf("\n=== NULL buffer handling ===\n");
	errno = 0;
	ret = itoa(123,NULL,10);
	printf("NULL buffer result: %s, errno: %d\n",ret ? "non-NULL" : "NULL",errno);
}

static Return capture_librational_itoa_output(void)
{
	INITTEST;

	test_itoa();

	deliver(status);
}

/**
 * @brief Check that the itoa() demonstration output remains unchanged
 *
 * @return Return describing success or failure
 */
static Return test_librational_0002_1(void)
{
	INITTEST;

	ASSERT(SUCCESS == match_function_output(
		expected_itoa_stdout_pattern,
		NULL,
		capture_librational_itoa_output));

	RETURN_STATUS;
}

/**
 * @brief Check itoa() conversions and error paths directly
 *
 * @details This block exercises the key textual invariants without relying
 * on demonstration text: INT_MIN and INT_MAX in base 10 (which exercise
 * the UB-avoidance path documented in rational_itoa.c), zero handling
 * across multiple bases, the unsigned bit-pattern representation of
 * negative values in bases other than 10, the use of letters A-Z by the
 * maximum supported base 36, and the documented error paths
 *
 * @return Return describing success or failure
 */
static Return test_librational_0002_2(void)
{
	INITTEST;

	char ibuf[64];  /* Large enough to hold a 32-bit value printed in base 2 (32 chars + terminator) */
	char *iret = NULL;

	/* Extreme values in base 10. INT_MIN exercises the UB-avoidance path inside itoa() */
	iret = itoa(INT_MAX,ibuf,10);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"2147483647"));

	iret = itoa(INT_MIN,ibuf,10);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"-2147483648"));

	iret = itoa(INT_MIN,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"80000000"));

	/* Zero must format as "0" regardless of base */
	iret = itoa(0,ibuf,10);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"0"));

	iret = itoa(0,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"0"));

	iret = itoa(0,ibuf,2);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"0"));

	/* Negative values in bases other than 10 must be printed as the unsigned bit pattern */
	iret = itoa(-1,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"FFFFFFFF"));

	iret = itoa(-1,ibuf,2);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"11111111111111111111111111111111"));

	iret = itoa(-12345,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"FFFFCFC7"));

	/* Common non-decimal bases must use the expected digits for positive values */
	iret = itoa(255,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"FF"));

	iret = itoa(255,ibuf,2);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"11111111"));

	iret = itoa(12345,ibuf,16);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"3039"));

	iret = itoa(12345,ibuf,8);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"30071"));

	/* Base 36 is the maximum supported base and must use letters A-Z for digits >= 10 */
	iret = itoa(35,ibuf,36);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"Z"));

	iret = itoa(12345,ibuf,36);
	ASSERT(iret == ibuf);
	ASSERT(0 == strcmp(ibuf,"9IX"));

	/* Error paths: invalid bases and NULL output buffer.
	 * buffer[0] is pre-set to a non-NUL sentinel before each call so the ASSERT
	 * below verifies that itoa() actually wrote the terminator on the error path,
	 * rather than just leaving the prior buffer contents intact */
	char buffer[8];
	char *result = NULL;

	errno = 0;
	buffer[0] = 'X';
	result = itoa(123,buffer,0);
	ASSERT(result == buffer);
	ASSERT(errno == EINVAL);
	ASSERT(buffer[0] == '\0');

	errno = 0;
	buffer[0] = 'Y';
	result = itoa(123,buffer,1);
	ASSERT(result == buffer);
	ASSERT(errno == EINVAL);
	ASSERT(buffer[0] == '\0');

	errno = 0;
	buffer[0] = 'Z';
	result = itoa(123,buffer,37);
	ASSERT(result == buffer);
	ASSERT(errno == EINVAL);
	ASSERT(buffer[0] == '\0');

	errno = 0;
	result = itoa(123,NULL,10);
	ASSERT(result == NULL);
	ASSERT(errno == EINVAL);

	RETURN_STATUS;
}

/**
 * @brief Run librational itoa conversion tests
 *
 * @details The suite runs in two independent blocks. The first captures
 * the stdout produced by test_itoa() and matches it against a strict
 * full-output regex. The second uses direct ASSERT checks for boundary
 * values, supported bases, negative values, and error paths
 *
 * @return Return describing success or failure
 */
Return test_librational_0002(void)
{
	INITTEST;

	/* The hex and binary representations of negative ints below assume 32-bit int.
	 * The supported targets (Linux x86_64) satisfy this; the static assertion
	 * prevents silent breakage on any future port to a target with a wider int */
	_Static_assert(sizeof(int) * CHAR_BIT == 32,"test_librational_0002 expects a 32-bit int for stable hex and binary expectations");

	TEST(test_librational_0002_1,"itoa() demonstration output matches the strict full pattern");
	TEST(test_librational_0002_2,"itoa() converts boundaries, bases, negatives and error paths");

	RETURN_STATUS;
}
