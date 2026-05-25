#include "test_libmem_utils.h"


/**
 * @brief Capture the negative resize case for a data descriptor with stale string metadata
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_data_resize_cache(void)
{
	INITTEST;

	unsigned char materialized_bytes[] = {1u,2u,3u,4u};
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_descriptor.data = materialized_bytes;
	invalid_descriptor.length = sizeof(materialized_bytes);
	invalid_descriptor.actually_allocated_bytes = sizeof(materialized_bytes);
	invalid_descriptor.string_length = 2;
	invalid_descriptor.is_string = false;

	ASSERT(FAILURE == m_resize(&invalid_descriptor,sizeof(materialized_bytes) + 4u));
	ASSERT(invalid_descriptor.data == materialized_bytes);
	ASSERT(invalid_descriptor.length == sizeof(materialized_bytes));
	ASSERT(invalid_descriptor.actually_allocated_bytes == sizeof(materialized_bytes));
	ASSERT(invalid_descriptor.string_length == 2);
	ASSERT(invalid_descriptor.is_string == false);

	deliver(status);
}

/**
 * @brief Check data-mode resize rejects stale string metadata
 *
 * @return Return describing success or failure
 */
Return test_libmem_0057(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0057[] =
		"\\A.*Data descriptor has non-zero string_length during resize.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0057,
		capture_libmem_inconsistent_data_resize_cache));

	RETURN_STATUS;
}
