#include "test_libmem_all.h"

/**
 * @brief Capture negative already-string cases for data-to-string conversion
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_data_to_string_string_mode(void)
{
	INITTEST;

	memory zero_sized_string = m_init(unsigned char,MEMORY_STRING);
	memory zero_length_cached_string = m_init(char,MEMORY_STRING);
	unsigned char materialized_string[] = {'a','b','\0'};
	memory invalid_string_descriptor = m_init(unsigned char,MEMORY_STRING);

	zero_sized_string.single_element_size = 0;
	zero_length_cached_string.string_length = 1;

	invalid_string_descriptor.data = materialized_string;
	invalid_string_descriptor.length = sizeof(materialized_string);
	invalid_string_descriptor.actually_allocated_bytes = sizeof(materialized_string);
	invalid_string_descriptor.string_length = sizeof(materialized_string);
	invalid_string_descriptor.is_string = true;

	ASSERT(FAILURE == m_to_string(&zero_sized_string));
	ASSERT(zero_sized_string.length == 0);
	ASSERT(zero_sized_string.string_length == 0);
	ASSERT(zero_sized_string.is_string == true);

	ASSERT(FAILURE == m_to_string(&zero_length_cached_string));
	ASSERT(zero_length_cached_string.length == 0);
	ASSERT(zero_length_cached_string.string_length == 1);
	ASSERT(zero_length_cached_string.is_string == true);

	ASSERT(FAILURE == m_to_string(&invalid_string_descriptor));
	ASSERT(invalid_string_descriptor.data == materialized_string);
	ASSERT(invalid_string_descriptor.length == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.actually_allocated_bytes == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.string_length == sizeof(materialized_string));
	ASSERT(invalid_string_descriptor.is_string == true);

	deliver(status);
}

/**
 * @brief Check data-to-string conversion rejection for inconsistent already-string metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0040(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0040[] =
	        "\\A.*Descriptor element size is zero during string conversion"
	        ".*String descriptor has non-zero string_length with zero length during string conversion"
	        ".*String descriptor cache is inconsistent during string conversion.*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0040,capture_libmem_inconsistent_data_to_string_string_mode));

	RETURN_STATUS;
}
