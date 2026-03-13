#include "sute.h"

/**
 * @brief Verify that a NULL-terminated string array matches expected contents
 *
 * @param[in] array Actual string array
 * @param[in] expected Expected strings in order
 * @param[in] expected_size Number of expected entries before the NULL terminator
 * @return Return status code
 */
static Return verify_array_contents(
	char       **array,
	const char **expected,
	size_t     expected_size)
{

	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	if((SUCCESS & status) && array == NULL)
	{
		status = FAILURE;
	}

	for(size_t i = 0; i < expected_size; i++)
	{
		if((SUCCESS & status) && array[i] == NULL)
		{
			status = FAILURE;
		}
		if((SUCCESS & status) && strcmp(array[i],expected[i]) != 0)
		{
			status = FAILURE;
		}
	}
	if((SUCCESS & status) && array[expected_size] != NULL)  // Verify NULL termination
	{
		status = FAILURE;
	}

	deliver(status);
}

static Return test0012_1(void)
{
	INITTEST;

	char **array = NULL;
	const char *test_string = "Hello World";

	ASSERT(SUCCESS == add_string_to_array(&array,test_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],test_string) == 0);
	ASSERT(array[1] == NULL);

	free_string_array(array);

	RETURN_STATUS;
}

/**
 * @brief Verify that repeated inserts preserve order in the allocated string array
 *
 * @return Return status code
 */
static Return test0012_2(void)
{
	INITTEST;

	char **array = NULL;
	const char *strings[] = {
		"First","Second","Third"
	};
	const size_t num_strings = sizeof(strings) / sizeof(strings[0]);

	for(size_t i = 0; i < num_strings; i++)
	{
		ASSERT(SUCCESS == add_string_to_array(&array,strings[i]));
	}

	ASSERT(SUCCESS & verify_array_contents(array,strings,num_strings));

	free_string_array(array);

	RETURN_STATUS;
}

static Return test0012_3(void)
{
	INITTEST;

	char **array = NULL;
	const char *empty_string = "";

	ASSERT(SUCCESS == add_string_to_array(&array,empty_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],empty_string) == 0);
	ASSERT(array[1] == NULL);

	free_string_array(array);

	RETURN_STATUS;
}

static Return test0012_4(void)
{
	INITTEST;

	char **array = NULL;
	char long_string[1024];
	memset(long_string,'A',sizeof(long_string) - 1);
	long_string[sizeof(long_string) - 1] = '\0';

	ASSERT(SUCCESS == add_string_to_array(&array,long_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],long_string) == 0);
	ASSERT(array[1] == NULL);

	free_string_array(array);

	RETURN_STATUS;
}

/**
 *
 * Unit Testing of precizer. add_string_to_array() function test set
 *
 */
Return test0012(void)
{
	INITTEST;

	TEST(test0012_1,"Adding string to empty array…");
	TEST(test0012_2,"Testing adding multiple strings…");
	TEST(test0012_3,"Testing adding empty string…");
	TEST(test0012_4,"Testing adding long string…");

	RETURN_STATUS;
}
