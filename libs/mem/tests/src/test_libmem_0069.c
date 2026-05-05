#include "test_libmem_utils.h"

static Return captured_status = SUCCESS;
static int captured_failed_line = 0;

static const char expected_stdout_pattern[] =
	"\\A"
	"delta!\n"
	"epsilon!\n"
	"zeta!\n"
	"\\Z";

static const char epsilon_bounded_source[] =
	{'e','p','s','i','l','o','n','\0','x'};

/**
 * @brief Capture inline string descriptors stored inside one root descriptor
 */
static void capture_inline_string_descriptor_table(void)
{
	INITTEST;

	m_create(int,numbers);
	m_create(memory,table);

	ASSERT(SUCCESS == m_resize(numbers,3));

	int *number_view = m_data(int,numbers);
	ASSERT(number_view != NULL);

	if(number_view != NULL)
	{
		number_view[0] = 1;
		number_view[1] = 2;
		number_view[2] = 3;
	}

	int *second_number = m_item(int,numbers,1);
	ASSERT(second_number != NULL);

	if(second_number != NULL)
	{
		ASSERT(*second_number == 2);
	}

	const int *third_number = m_item_ro(int,numbers,2);
	ASSERT(third_number != NULL);

	if(third_number != NULL)
	{
		ASSERT(*third_number == 3);
	}

	size_t number_count = 0;
	int number_sum = 0;

	mem_core_array_foreach(numbers,int,number)
	{
		*number += 1;
		number_sum += *number;
		++number_count;
	}

	ASSERT(number_count == 3U);
	ASSERT(number_sum == 9);

	if(number_view != NULL)
	{
		ASSERT(number_view[0] == 2);
		ASSERT(number_view[1] == 3);
		ASSERT(number_view[2] == 4);
	}

	ASSERT(SUCCESS == m_string_array_append(table,char,"delta"));
	ASSERT(SUCCESS == m_string_array_append(
		table,
		char,
		sizeof(epsilon_bounded_source),
		epsilon_bounded_source));
	ASSERT(SUCCESS == m_string_array_append(table,char,"zeta"));
	ASSERT(table->is_string == false);
	ASSERT(table->single_element_size == sizeof(memory));
	ASSERT(table->length == 3);

	const memory *epsilon_descriptor = m_item_ro(memory,table,1);
	ASSERT(epsilon_descriptor != NULL);

	if(epsilon_descriptor != NULL)
	{
		ASSERT(strcmp(m_text(epsilon_descriptor),"epsilon") == 0);
	}

	size_t descriptor_count = 0;

	m_string_array_foreach(table,descriptor)
	{
		ASSERT(descriptor->is_string == true);
		ASSERT(descriptor->single_element_size == sizeof(char));
		ASSERT(SUCCESS == m_concat_literal(descriptor,"!"));
		++descriptor_count;
	}

	ASSERT(descriptor_count == table->length);

	mem_core_array_foreach(table,memory,descriptor)
	{
		ASSERT(descriptor->is_string == true);
		ASSERT(descriptor->single_element_size == sizeof(char));
		printf("%s\n",m_text(descriptor));
	}

	ASSERT(SUCCESS == m_del(numbers));
	ASSERT(SUCCESS == m_array_del(table));
	ASSERT(table->data == NULL);
	ASSERT(table->length == 0);
	ASSERT(table->string_length == 0);
	ASSERT(table->is_string == false);
	ASSERT(table->single_element_size == sizeof(memory));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Capture negative item-access cases for descriptor-backed arrays
 */
static void capture_array_item_negative_cases(void)
{
	INITTEST;

	m_create(int,numbers);
	m_create(int,empty);

	ASSERT(SUCCESS == m_resize(numbers,2));

	ASSERT(NULL == m_item_ro(int,NULL,0));
	ASSERT(NULL == m_item(int,NULL,0));
	ASSERT(NULL == m_item_ro(int,empty,0));
	ASSERT(NULL == m_item(int,empty,0));
	ASSERT(NULL == m_item_ro(int,numbers,2));
	ASSERT(NULL == m_item(int,numbers,2));
	ASSERT(NULL == m_item_ro(unsigned long long,numbers,0));
	ASSERT(NULL == m_item(unsigned long long,numbers,0));

	ASSERT(SUCCESS == m_del(numbers));
	ASSERT(SUCCESS == m_del(empty));

	captured_status = status;
	captured_failed_line = failed_line;
}

/**
 * @brief Check inline string descriptors in one root descriptor
 *
 * @return Return describing success or failure
 */
Return test_libmem_0069(void)
{
	INITTEST;

	m_create(char,captured_stdout,MEMORY_STRING);
	m_create(char,captured_stderr,MEMORY_STRING);
	m_create(char,negative_stdout,MEMORY_STRING);
	m_create(char,negative_stderr,MEMORY_STRING);
	m_create(char,pattern,MEMORY_STRING);
	Return capture_status = SUCCESS;
	const char *pattern_label = "inline string descriptor table stdout pattern";

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_inline_string_descriptor_table,
		captured_stdout,
		captured_stderr);

	if(capture_status != SUCCESS)
	{
		captured_status = capture_status;
		captured_failed_line = __LINE__;
	}

	ASSERT(SUCCESS == capture_status);
	ASSERT(captured_stderr->length == 0);
	ASSERT(SUCCESS == m_copy_fixed_string(pattern,sizeof(expected_stdout_pattern),expected_stdout_pattern));
	ASSERT(SUCCESS == match_pattern(captured_stdout,pattern,pattern_label));

	captured_status = FAILURE;
	captured_failed_line = 0;

	capture_status = function_capture(
		capture_array_item_negative_cases,
		negative_stdout,
		negative_stderr);

	if(capture_status != SUCCESS)
	{
		captured_status = capture_status;
		captured_failed_line = __LINE__;
	}

	ASSERT(SUCCESS == capture_status);
	ASSERT(negative_stdout->length == 0);
	ASSERT(negative_stderr->length > 0);

	const char *negative_report = m_text(negative_stderr);
	ASSERT(negative_report != NULL);

	if(negative_report != NULL)
	{
		ASSERT(strstr(negative_report,"Descriptor is NULL") != NULL);
		ASSERT(strstr(negative_report,"Item index 0 is out of range for descriptor length 0") != NULL);
		ASSERT(strstr(negative_report,"Item index 2 is out of range for descriptor length 2") != NULL);
		ASSERT(strstr(negative_report,"Expected") != NULL);
	}

	call(m_del(pattern));
	call(m_del(negative_stderr));
	call(m_del(negative_stdout));
	call(m_del(captured_stderr));
	call(m_del(captured_stdout));

	failed_line = captured_failed_line;
	status = captured_status;

	RETURN_STATUS;
}
