#include "precizer.h"
#include "sute.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test helper function to free the array
static void free_test_array(char **array){
	if(array == NULL)
	{
		return;
	}

	for(size_t i = 0; array[i] != NULL; i++)
	{
		free(array[i]);
	}
	free(array);
}

// Test helper function to verify array contents
static Return verify_array_contents(
	char       **array,
	const char **expected,
	size_t     expected_size
){

	Return status = SUCCESS;

	ASSERT(array != NULL);

	for(size_t i = 0; i < expected_size; i++)
	{
		ASSERT(array[i] != NULL);
		ASSERT(strcmp(array[i],expected[i]) == 0);
	}
	ASSERT(array[expected_size] == NULL);  // Verify NULL termination

	return(status);
}

static Return test_add_string_to_empty_array(void){
	Return status = SUCCESS;

	char **array = NULL;
	const char *test_string = "Hello World";

	ASSERT(SUCCESS == add_string_to_array(&array,test_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],test_string) == 0);
	ASSERT(array[1] == NULL);

	free_test_array(array);

	RETURN_STATUS;
}

static Return test_add_multiple_strings(void){
	Return status = SUCCESS;

	char **array = NULL;
	const char *strings[] = {
		"First","Second","Third"
	};
	const size_t num_strings = sizeof(strings) / sizeof(strings[0]);

	for(size_t i = 0; i < num_strings; i++)
	{
		ASSERT(SUCCESS == add_string_to_array(&array,strings[i]));
	}

	ASSERT(SUCCESS == verify_array_contents(array,strings,num_strings));

	free_test_array(array);

	RETURN_STATUS;
}

static Return test_add_empty_string(void){
	Return status = SUCCESS;

	char **array = NULL;
	const char *empty_string = "";

	ASSERT(SUCCESS == add_string_to_array(&array,empty_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],empty_string) == 0);
	ASSERT(array[1] == NULL);

	free_test_array(array);

	RETURN_STATUS;
}

static Return test_add_long_string(void){
	Return status = SUCCESS;

	char **array = NULL;
	char long_string[1024];
	memset(long_string,'A',sizeof(long_string) - 1);
	long_string[sizeof(long_string) - 1] = '\0';

	ASSERT(SUCCESS == add_string_to_array(&array,long_string));
	ASSERT(array != NULL);
	ASSERT(array[0] != NULL);
	ASSERT(strcmp(array[0],long_string) == 0);
	ASSERT(array[1] == NULL);

	free_test_array(array);

	RETURN_STATUS;
}

// Main test runner
Return test0012(void){
	/// The status that will be passed to return() before exiting.
	/// By default, the function worked without errors.
	Return status = SUCCESS;

	TEST(test_add_string_to_empty_array,"Adding string to empty array…");
	TEST(test_add_multiple_strings,"Testing adding multiple strings…");
	TEST(test_add_empty_string,"Testing adding empty string…");
	TEST(test_add_long_string,"Testing adding long string…");

	RETURN_STATUS;
}
