#include "test_libmem_utils.h"


/**
 * @brief Capture the zero-sized string-length negative case
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_zero_sized_element_string_length(void)
{
	INITTEST;

	size_t measured_length = SIZE_MAX;
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_descriptor.single_element_size = 0;

	invalid_descriptor.length = 1;

	ASSERT(FAILURE == m_string_length(&invalid_descriptor,&measured_length));
	ASSERT(measured_length == SIZE_MAX);

	deliver(status);
}

/**
 * @brief Check string_length rejects zero-sized elements
 *
 * @return Return describing success or failure
 */
Return test_libmem_0024(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0024[] =
		"\\A.*Descriptor has non-zero length with NULL data pointer.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0024,
		capture_libmem_zero_sized_element_string_length));

	RETURN_STATUS;
}
