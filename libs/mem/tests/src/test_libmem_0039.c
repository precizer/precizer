#include "test_libmem_all.h"

/**
 * @brief Capture the negative data-to-string conversion case for stale data-mode string metadata
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_data_to_string_cache(void)
{
	INITTEST;

	unsigned char materialized_bytes[] = {'a','b','c','\0'};
	memory invalid_data_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_data_descriptor.data = materialized_bytes;
	invalid_data_descriptor.length = sizeof(materialized_bytes);
	invalid_data_descriptor.actually_allocated_bytes = sizeof(materialized_bytes);
	invalid_data_descriptor.string_length = 2;
	invalid_data_descriptor.is_string = false;

	ASSERT(FAILURE == m_to_string(&invalid_data_descriptor));
	ASSERT(invalid_data_descriptor.data == materialized_bytes);
	ASSERT(invalid_data_descriptor.length == sizeof(materialized_bytes));
	ASSERT(invalid_data_descriptor.actually_allocated_bytes == sizeof(materialized_bytes));
	ASSERT(invalid_data_descriptor.string_length == 2);
	ASSERT(invalid_data_descriptor.is_string == false);

	deliver(status);
}

/**
 * @brief Check data-to-string conversion rejection for stale string metadata in data mode
 *
 * @return Return describing success or failure
 */
Return test_libmem_0039(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0039[] =
	        "\\A.*Data descriptor has non-zero string_length during string conversion.*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0039,capture_libmem_inconsistent_data_to_string_cache));

	RETURN_STATUS;
}
