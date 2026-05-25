#include "test_libmem_utils.h"


/**
 * @brief Capture the zero-sized string-to-data negative case
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_zero_sized_element_string_to_data(void)
{
	INITTEST;

	memory invalid_string = m_init(unsigned char,MEMORY_STRING);

	invalid_string.single_element_size = 0;

	invalid_string.length = 1;

	ASSERT(FAILURE == m_to_data(&invalid_string));
	ASSERT(invalid_string.length == 1);
	ASSERT(invalid_string.string_length == 0);
	ASSERT(invalid_string.is_string == true);

	deliver(status);
}

/**
 * @brief Check string-to-data conversion rejects zero-sized elements
 *
 * @return Return describing success or failure
 */
Return test_libmem_0022(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0022[] =
		"\\A.*Descriptor has non-zero length with NULL data pointer.*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0022,capture_libmem_zero_sized_element_string_to_data));

	RETURN_STATUS;
}
