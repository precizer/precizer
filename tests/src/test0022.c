#include "sute.h"

/**
 * @brief Helper function to test remove_trailing_slash and compare with expected output.
 *
 * This function copies the input string, applies remove_trailing_slash(), and
 * compares the result with the expected output.
 *
 * @param input The original path to be processed.
 * @param expected The expected output after removing trailing slashes.
 */
static Return test_remove_trailing_slash(
	const char *input,
	const char *expected)
{
	INITTEST;

	create(char,temp_buffer);
	size_t len = strlen(input) + 1; // +1 for null terminator

	ASSERT(SUCCESS == resize(temp_buffer,len));

	if(SUCCESS == status)
	{
		char *buffer_data = data(char,temp_buffer);

		if(buffer_data == NULL)
		{
			status = FAILURE;
		} else {
			memcpy(buffer_data,input,len);
			remove_trailing_slash(buffer_data);

			const char *buffer_view = cdata(char,temp_buffer);

			if(buffer_view == NULL)
			{
				status = FAILURE;
			} else {
				ASSERT(COMPLETED == strcmp(buffer_view,expected));
			}
		}
	}

	del(temp_buffer);

	return(status);
}

/**
 * @brief Runs a series of test cases for remove_trailing_slash().
 *
 * Unit Testing of precizer. This function tests various scenarios including:
 * - Removing trailing slashes from paths
 * - Ensuring single '/' paths remain unchanged
 * - Handling edge cases with multiple slashes
 */
Return test0022(void)
{
	INITTEST;

	static const struct {
		const char *input;
		const char *expected;
	} test_cases[] = {
		// Basic cases
		{"path/","path"},
		{"folder////","folder"},
		{"/home/user////","/home/user"},
		{"/var/log//","/var/log"},
		{"/usr/local/bin/","/usr/local/bin"},
		{"///home///","///home"},

		// Root path cases
		{"/","/"},
		{"////","/"},

		// No trailing slashes
		{"path","path"},
		{"/usr/local/bin","/usr/local/bin"},
		{"no/slash/here","no/slash/here"},

		// Empty string
		{"",""},

		// Edge cases
		{"//double//slash//","//double//slash"},
		{"////multiple///leading/slashes////","////multiple///leading/slashes"},

		// Single character paths
		{"a/","a"},
		{"b////","b"},
		{"c","c"},
		{NULL,NULL} // Sentinel value
	};

	for(size_t i = 0; test_cases[i].input != NULL; i++)
	{
		ASSERT(SUCCESS == test_remove_trailing_slash(test_cases[i].input,test_cases[i].expected));
	}

	RETURN_STATUS;
}
