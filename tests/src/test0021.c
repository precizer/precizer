#include "sute.h"

static void test0021_1(void)
{

	setlocale(LC_ALL,""); // Enable UTF-8 support

	char test1[] = "...Пример строки...";
	char test2[] = "....Тест...";
	char test3[] = "Просто текст";
	char test4[] = "......Только точки......";
	char test5[] = "...";
	char test6[] = "Без точек";
	char test7[] = "...Останутся только точки справа...";
	char test8[] = "...Останутся только точки слева...";

	remove_leading_dots(test1);
	remove_trailing_dots(test1);
	remove_leading_dots(test2);
	remove_trailing_dots(test2);
	remove_leading_dots(test3);
	remove_trailing_dots(test3);
	remove_leading_dots(test4);
	remove_trailing_dots(test4);
	remove_leading_dots(test5);
	remove_trailing_dots(test5);
	remove_leading_dots(test6);
	remove_trailing_dots(test6);
	remove_leading_dots(test7);
	remove_trailing_dots(test8);

	printf("Result 1: \"%s\"\n",test1);
	printf("Result 2: \"%s\"\n",test2);
	printf("Result 3: \"%s\"\n",test3);
	printf("Result 4: \"%s\"\n",test4);
	printf("Result 5: \"%s\"\n",test5);
	printf("Result 6: \"%s\"\n",test6);
	printf("Result 7: \"%s\"\n",test7);
	printf("Result 8: \"%s\"\n",test8);

	// Disable locale
	setlocale(LC_ALL,"C");
}

/**
 *
 * @brief UTF8 manipulations
 *
 */
Return test0021(void)
{
	INITTEST;

	create_mem(mem_char,captured_stdout);
	create_mem(mem_char,captured_stderr);

	char *pattern = NULL;

	ASSERT(SUCCESS == function_capture(test0021_1,captured_stdout,captured_stderr));

	if(captured_stderr->length > 0)
	{
		echo(STDERR,"ERROR: Stderr buffer is not empty. It contains characters: %zu\n",captured_stderr->length);
		status = FAILURE;
	}

	ASSERT(SUCCESS == get_file_content("templates/0021_0001.txt",&pattern));

	// Match the result against the pattern
	ASSERT(SUCCESS == match_pattern(captured_stdout->mem,pattern));

	reset(&pattern);

	del_char(&captured_stdout);
	del_char(&captured_stderr);

	RETURN_STATUS;
}
