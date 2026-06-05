#include "test_libmem_all.h"

/**
 * @brief Capture the negative resize case for a data descriptor whose reserve is too small
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_data_resize_reserve(void)
{
	INITTEST;

	unsigned char materialized_bytes[] = {1u,2u,3u,4u};
	memory invalid_descriptor = m_init(unsigned char,MEMORY_DATA);

	invalid_descriptor.data = materialized_bytes;
	invalid_descriptor.length = sizeof(materialized_bytes);
	invalid_descriptor.actually_allocated_bytes = sizeof(materialized_bytes) - 1u;
	invalid_descriptor.string_length = 0;
	invalid_descriptor.is_string = false;

	ASSERT(FAILURE == m_resize(&invalid_descriptor,sizeof(materialized_bytes)));
	ASSERT(invalid_descriptor.data == materialized_bytes);
	ASSERT(invalid_descriptor.length == sizeof(materialized_bytes));
	ASSERT(invalid_descriptor.actually_allocated_bytes == sizeof(materialized_bytes) - 1u);
	ASSERT(invalid_descriptor.string_length == 0);
	ASSERT(invalid_descriptor.is_string == false);

	deliver(status);
}

/**
 * @brief Check data-mode resize rejects descriptors whose reserve does not cover the payload
 *
 * @return Return describing success or failure
 */
Return test_libmem_0058(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0058[] =
	        "\\A.*Descriptor reserve is smaller than logical payload during resize.*\\Z";

	ASSERT(SUCCESS == match_function_output(NULL,expected_stderr_pattern_libmem_0058,capture_libmem_inconsistent_data_resize_reserve));

	RETURN_STATUS;
}
