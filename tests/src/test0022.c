#include "sute.h"

static const char expected_noisy_remove_trailing_slash_stdout_pattern[] =
        "\\A"
        "ERROR: Path normalization; Failed to access non-empty descriptor as a char buffer\n"
        "\\Z";

static const char expected_noisy_remove_trailing_slash_stderr_pattern[] =
        "\\A"
        "ERROR: [^\\n]*Memory management; Invalid arguments for string length helper[^\\n]*\n"
        "ERROR: [^\\n]*Memory management; Expected 1 bytes but descriptor uses 4[^\\n]*\n"
        "ERROR: [^\\n]*Memory management; Descriptor has non-zero length with NULL data pointer[^\\n]*\n"
        "\\Z";

static memory *noisy_non_string_path = NULL;
static memory *noisy_wrong_type_path = NULL;
static memory *noisy_invalid_path = NULL;
static Return noisy_null_status = SUCCESS;
static Return noisy_non_string_status = SUCCESS;
static Return noisy_wrong_type_status = SUCCESS;
static Return noisy_invalid_path_status = SUCCESS;

/**
 * @brief Exercise descriptor edge cases for remove_trailing_slash() under output capture
 *
 * The helper is invoked through match_function_output() so expected diagnostics
 * stay captured instead of leaking into the real terminal output. The individual
 * function statuses are stored in file-scope variables so the outer test
 * function can assert on them after output matching succeeds
 *
 * @return SUCCESS when all negative calls were exercised
 */
static Return capture_noisy_remove_trailing_slash_cases(void)
{
	/* Status returned by this function through deliver()
	   Default value assumes successful completion */
	Return status = SUCCESS;

	noisy_null_status = remove_trailing_slash(NULL);
	noisy_non_string_status = remove_trailing_slash(noisy_non_string_path);
	noisy_wrong_type_status = remove_trailing_slash(noisy_wrong_type_path);
	noisy_invalid_path_status = remove_trailing_slash(noisy_invalid_path);

	deliver(status);
}

/**
 * @brief Helper function to test remove_trailing_slash and compare with expected output
 *
 * This function copies the input string, applies remove_trailing_slash(), and
 * compares the result with the expected output
 *
 * @param input The original path to be processed
 * @param expected The expected output after removing trailing slashes
 */
static Return test_remove_trailing_slash(
	const char *input,
	const char *expected)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,temp_buffer,MEMORY_STRING);
	const size_t expected_length = strlen(expected) + 1;

	ASSERT(SUCCESS == m_copy_string(temp_buffer,input));
	ASSERT(SUCCESS & remove_trailing_slash(temp_buffer));

	const char *buffer_view = m_text(temp_buffer);

	ASSERT(buffer_view != NULL);
	ASSERT(0 == strcmp(buffer_view,expected));
	ASSERT(temp_buffer->string_length == expected_length - 1U);
	ASSERT(temp_buffer->length == expected_length);

	m_del(temp_buffer);

	deliver(status);
}

/**
 * @brief Helper function to test remove_trailing_slash against a raw char data descriptor
 *
 * @param input_data Raw input bytes to place into the descriptor
 * @param input_size Number of bytes in @p input_data
 * @param expected The expected C string after removing trailing slashes
 * @return Test status indicating success or failure
 */
static Return test_remove_trailing_slash_data(
	const char *input_data,
	size_t     input_size,
	const char *expected)
{
	/* The status that will be returned before exiting */
	/* By default, assumes the function ran without errors */
	Return status = SUCCESS;

	m_create(char,temp_buffer,MEMORY_DATA);
	const size_t expected_length = strlen(expected) + 1;

	ASSERT(input_data != NULL);
	ASSERT(SUCCESS == m_copy_buffer(temp_buffer,input_size,input_data));
	ASSERT(SUCCESS & remove_trailing_slash(temp_buffer));

	char *buffer_data = m_data(char,temp_buffer);

	ASSERT(buffer_data != NULL);
	ASSERT(0 == strcmp(buffer_data,expected));
	ASSERT(temp_buffer->string_length == 0);
	ASSERT(temp_buffer->length == expected_length);

	m_del(temp_buffer);

	deliver(status);
}

/**
 * @brief Runs a series of test cases for remove_trailing_slash()
 *
 * Unit Testing of precizer. This function tests various scenarios including:
 * - Removing trailing slashes from paths
 * - Ensuring single '/' paths remain unchanged
 * - Handling edge cases with multiple slashes
 * - Accepting managed char data descriptors that contain a bounded string payload
 * - Compacting descriptor length to visible string plus one terminator
 * - Returning FAILURE for NULL, wrong-width, and data-less non-empty descriptors
 * - Preserving graceful propagated statuses while still trimming
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
		ASSERT(SUCCESS & test_remove_trailing_slash(test_cases[i].input,test_cases[i].expected));
	}

	static const char data_path_without_terminator[] = {
		'd','a','t','a'
	};

	static const char slashy_data_path_without_terminator[] = {
		'd','a','t','a','/','/'
	};

	ASSERT(SUCCESS & test_remove_trailing_slash_data(
		data_path_without_terminator,
		sizeof(data_path_without_terminator),
		"data"));
	ASSERT(SUCCESS & test_remove_trailing_slash_data(
		slashy_data_path_without_terminator,
		sizeof(slashy_data_path_without_terminator),
		"data"));

	m_create(char,non_string_path,MEMORY_DATA);
	ASSERT(SUCCESS == m_copy_buffer(non_string_path,sizeof("data////"),"data////"));

	int wrong_type_value = 0;
	memory wrong_type_path = {
		.single_element_size = sizeof(int),
		.actually_allocated_bytes = sizeof(int),
		.length = 1,
		.string_length = 1,
		.is_string = true,
		.data = &wrong_type_value
	};

	memory invalid_path = {
		.single_element_size = sizeof(char),
		.actually_allocated_bytes = 1,
		.length = 1,
		.string_length = 0,
		.is_string = true,
		.data = NULL
	};

	noisy_non_string_path = non_string_path;
	noisy_wrong_type_path = &wrong_type_path;
	noisy_invalid_path = &invalid_path;
	noisy_null_status = SUCCESS;
	noisy_non_string_status = SUCCESS;
	noisy_wrong_type_status = SUCCESS;
	noisy_invalid_path_status = SUCCESS;

	ASSERT(SUCCESS == match_function_output(
		expected_noisy_remove_trailing_slash_stdout_pattern,
		expected_noisy_remove_trailing_slash_stderr_pattern,
		capture_noisy_remove_trailing_slash_cases));
	ASSERT(FAILURE & noisy_null_status);
	ASSERT(TRIUMPH & noisy_non_string_status);
	ASSERT(FAILURE & noisy_wrong_type_status);
	ASSERT(FAILURE & noisy_invalid_path_status);
	char *non_string_path_data = m_data(char,non_string_path);

	ASSERT(non_string_path_data != NULL);
	IF(SUCCESS == status)
	{
		ASSERT(0 == strcmp(non_string_path_data,"data"));
		ASSERT(non_string_path->length == sizeof("data"));
		ASSERT(non_string_path->string_length == 0);
	}
	m_del(non_string_path);

	m_create(char,hidden_tail_path,MEMORY_STRING);

	static const char hidden_tail_bytes[] = {
		'a','b','c','\0','g','a','r','b','a','g','e','/','/'
	};
	char *hidden_tail_data = NULL;

	ASSERT(SUCCESS == m_resize(hidden_tail_path,sizeof(hidden_tail_bytes)));
	hidden_tail_data = m_data(char,hidden_tail_path);
	ASSERT(hidden_tail_data != NULL);
	IF(SUCCESS == status)
	{
		memcpy(hidden_tail_data,hidden_tail_bytes,sizeof(hidden_tail_bytes));
	}
	ASSERT(SUCCESS == m_finalize_string(hidden_tail_path,sizeof("abc") - 1U));
	ASSERT(0 == memcmp(hidden_tail_path->data,hidden_tail_bytes,sizeof(hidden_tail_bytes)));
	ASSERT(SUCCESS & remove_trailing_slash(hidden_tail_path));
	ASSERT(0 == strcmp(m_text(hidden_tail_path),"abc"));
	ASSERT(hidden_tail_path->string_length == strlen("abc"));
	ASSERT(hidden_tail_path->length == sizeof("abc"));

	m_del(hidden_tail_path);

	m_create(char,slashy_path,MEMORY_STRING);

	ASSERT(SUCCESS == m_copy_literal(slashy_path,"abc//"));
	ASSERT(SUCCESS & remove_trailing_slash(slashy_path));
	ASSERT(0 == strcmp(m_text(slashy_path),"abc"));
	ASSERT(slashy_path->string_length == strlen("abc"));
	ASSERT(slashy_path->length == sizeof("abc"));

	m_del(slashy_path);

	m_create(char,graceful_path,MEMORY_STRING);
	ASSERT(SUCCESS == m_copy_literal(graceful_path,"graceful////"));

	IF(SUCCESS == status)
	{
		Return saved_global_status = atomic_exchange(&global_return_status,INFO);
		Return graceful_status = remove_trailing_slash(graceful_path);
		atomic_store(&global_return_status,saved_global_status);

		ASSERT(TRIUMPH & graceful_status);
		ASSERT(INFO & graceful_status);
		ASSERT(0 == strcmp(m_text(graceful_path),"graceful"));
		ASSERT(graceful_path->string_length == strlen("graceful"));
		ASSERT(graceful_path->length == sizeof("graceful"));
	}

	m_del(graceful_path);

	RETURN_STATUS;
}
