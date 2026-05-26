#include "test_libmem_utils.h"


/**
 * @brief Capture the zero-sized string-to-data negative case
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_zero_sized_element_string_to_data(void)
{
	INITTEST;

	unsigned char payload[] = {0U};
	memory invalid_string = m_init(unsigned char,MEMORY_STRING);

	/* Keep the backing storage valid so this test reaches only the
	   deliberately invalid zero-sized element condition */
	invalid_string.data = payload;
	invalid_string.actually_allocated_bytes = sizeof(payload);
	invalid_string.length = 1;
	invalid_string.single_element_size = 0;

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
		"\\A.*Descriptor element size is zero \\(uninitialized\\).*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0022,capture_libmem_zero_sized_element_string_to_data));

	RETURN_STATUS;
}
