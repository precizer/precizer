#include "test_libmem_all.h"

/**
 * @brief Capture inline string descriptors stored inside one root descriptor
 *
 * @return Return describing success or failure
 */
static Return capture_inline_string_descriptor_table(void)
{
	INITTEST;

	m_create(int,numbers);
	m_create(memory,table);

	ASSERT(SUCCESS == m_resize(numbers,3));

	int *number_view = m_data(int,numbers);
	ASSERT(number_view != NULL);

	IF(number_view != NULL)
	{
		number_view[0] = 1;
		number_view[1] = 2;
		number_view[2] = 3;
	}

	int *second_number = m_item(int,numbers,1);
	ASSERT(second_number != NULL);

	IF(second_number != NULL)
	{
		ASSERT(*second_number == 2);
	}

	const int *third_number = m_item_ro(int,numbers,2);
	ASSERT(third_number != NULL);

	IF(third_number != NULL)
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

	IF(number_view != NULL)
	{
		ASSERT(number_view[0] == 2);
		ASSERT(number_view[1] == 3);
		ASSERT(number_view[2] == 4);
	}

	static const char epsilon_bounded_source[] =
	{'e','p','s','i','l','o','n','\0','x'};

	ASSERT(SUCCESS == m_string_array_append(table,char,"delta"));
	ASSERT(SUCCESS == m_string_array_append(table,char,sizeof(epsilon_bounded_source),epsilon_bounded_source));
	ASSERT(SUCCESS == m_string_array_append(table,char,"zeta"));
	ASSERT(table->is_string == false);
	ASSERT(table->single_element_size == sizeof(memory));
	ASSERT(table->length == 3);

	const memory *epsilon_descriptor = m_item_ro(memory,table,1);
	ASSERT(epsilon_descriptor != NULL);

	IF(epsilon_descriptor != NULL)
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

	m_string_array_foreach(table,descriptor)
	{
		ASSERT(descriptor->is_string == true);
		ASSERT(descriptor->single_element_size == sizeof(char));
		printf("%s\n",m_text(descriptor));
	}

	call(m_del(numbers));
	ASSERT(SUCCESS == m_array_del(table));
	ASSERT(table->data == NULL);
	ASSERT(table->length == 0);
	ASSERT(table->string_length == 0);
	ASSERT(table->is_string == false);
	ASSERT(table->single_element_size == sizeof(memory));

	deliver(status);
}

/**
 * @brief Capture negative item-access cases for descriptor-backed arrays
 *
 * @return Return describing success or failure
 */
static Return capture_array_item_negative_cases(void)
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

	call(m_del(numbers));
	call(m_del(empty));

	deliver(status);
}

/**
 * @brief Check inline string descriptors in one root descriptor
 *
 * @return Return describing success or failure
 */
Return test_libmem_0069(void)
{
	INITTEST;

	static const char expected_stdout_pattern[] =
	        "\\A"
	        "delta!\n"
	        "epsilon!\n"
	        "zeta!\n"
	        "\\Z";

	static const char expected_negative_stderr_pattern[] =
	        "\\A.*Descriptor is NULL"
	        ".*Item index 0 is out of range for descriptor length 0"
	        ".*Item index 2 is out of range for descriptor length 2"
	        ".*Expected.*\\Z";

	ASSERT(SUCCESS == match_function_output(expected_stdout_pattern,NULL,capture_inline_string_descriptor_table));

	ASSERT(SUCCESS == match_function_output(NULL,expected_negative_stderr_pattern,capture_array_item_negative_cases));

	RETURN_STATUS;
}
