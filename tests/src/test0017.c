#include "sute.h"
#include <limits.h>

static void test_conversion(
	int  value,
	int  base,
	char *string
){
	char buffer[66];  /* 64 bits + sign + null terminator */
	itoa(value,buffer,base);

	/* Print original value in decimal and result in specified base */
	printf("Value: %d (decimal)\n",value);
	printf("Base %2d result: %s, %s\n",base,buffer,string);
	printf("-------------------\n");
}

/**
 * @brief Test program for itoa function
 *
 * @note Tests edge cases and different bases with special focus on
 *       negative numbers and MIN/MAX integer values
 */
Return test_itoa(void){

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

	return 0; // Success
}

Return test0017(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	MSTRUCT(mem_char,captured_stdout);
	MSTRUCT(mem_char,captured_stderr);

	char *pattern = NULL;

	ASSERT(SUCCESS == function_capture(test_itoa,captured_stdout,captured_stderr));

	if(captured_stderr->length > 0)
	{
		echo(STDERR,"ERROR: Stderr buffer is not empty. It contains characters: %zu\n",captured_stderr->length);
		status = FAILURE;
	}

	ASSERT(SUCCESS == get_file_content("templates/0017.txt",&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(captured_stdout->mem,pattern));

	free(pattern);
	pattern = NULL;

	del_char(&captured_stdout);
	del_char(&captured_stderr);

	RETURN_STATUS;
}
