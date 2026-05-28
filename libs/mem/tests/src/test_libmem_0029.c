#include "test_libmem_all.h"

/**
 * @brief Capture negative cases for descriptors with non-zero length and NULL data
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_string_descriptor(void)
{
	INITTEST;

	size_t measured_length = SIZE_MAX;
	memory invalid_data_descriptor = m_init(char,MEMORY_DATA);
	memory invalid_string_descriptor = m_init(char,MEMORY_STRING);

	invalid_data_descriptor.length = 3;
	invalid_string_descriptor.length = 3;
	invalid_string_descriptor.string_length = 2;

	ASSERT(FAILURE == m_string_length(&invalid_data_descriptor,&measured_length));
	ASSERT(measured_length == SIZE_MAX);
	ASSERT(FAILURE == m_to_string(&invalid_data_descriptor));
	ASSERT(FAILURE == m_to_data(&invalid_string_descriptor));

	const unsigned char *safe_string_view = (const unsigned char *)m_string(&invalid_string_descriptor);
	ASSERT(safe_string_view != NULL);

	IF(safe_string_view != NULL)
	{
		ASSERT(safe_string_view[0] == 0U);
	}

	ASSERT(invalid_data_descriptor.length == 3);
	ASSERT(invalid_data_descriptor.data == NULL);
	ASSERT(invalid_data_descriptor.is_string == false);
	ASSERT(invalid_string_descriptor.length == 3);
	ASSERT(invalid_string_descriptor.data == NULL);
	ASSERT(invalid_string_descriptor.is_string == true);

	deliver(status);
}

/**
 * @brief Check string-facing helpers reject inconsistent descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0029(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0029[] =
	        "\\A.*Descriptor has non-zero length with NULL data pointer.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0029,
		capture_libmem_inconsistent_string_descriptor));

	RETURN_STATUS;
}
