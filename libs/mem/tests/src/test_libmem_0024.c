#include "test_libmem_all.h"

/**
 * @brief Capture the zero-sized string-length negative case
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_zero_sized_element_string_length(void)
{
	INITTEST;

	size_t measured_length = SIZE_MAX;
	unsigned char payload[] = {0U};
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	/* Keep the backing storage valid so this test reaches only the
	   deliberately invalid zero-sized element condition */
	invalid_descriptor.data = payload;
	invalid_descriptor.actually_allocated_bytes = sizeof(payload);
	invalid_descriptor.length = 1;
	invalid_descriptor.single_element_size = 0;

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
	        "\\A.*Descriptor element size is zero \\(uninitialized\\).*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0024,
		capture_libmem_zero_sized_element_string_length));

	RETURN_STATUS;
}
