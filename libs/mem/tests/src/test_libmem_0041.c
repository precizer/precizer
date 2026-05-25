#include "test_libmem_utils.h"


/**
 * @brief Capture the negative resize case for a string descriptor whose reserve is too small
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_resize_string_reserve(void)
{
	INITTEST;

	char materialized_string[] = "alpha";
	memory invalid_string = m_init(char,MEMORY_STRING);

	invalid_string.data = materialized_string;
	invalid_string.length = sizeof(materialized_string);
	invalid_string.actually_allocated_bytes = sizeof(materialized_string) - 1u;
	invalid_string.string_length = sizeof(materialized_string) - 1u;
	invalid_string.is_string = true;

	ASSERT(FAILURE == m_resize(&invalid_string,sizeof(materialized_string)));
	ASSERT(invalid_string.data == materialized_string);
	ASSERT(invalid_string.length == sizeof(materialized_string));
	ASSERT(invalid_string.actually_allocated_bytes == sizeof(materialized_string) - 1u);
	ASSERT(invalid_string.string_length == sizeof(materialized_string) - 1u);
	ASSERT(invalid_string.is_string == true);

	deliver(status);
}

/**
 * @brief Check resize rejection for string descriptors whose reserve does not cover the payload
 *
 * @return Return describing success or failure
 */
Return test_libmem_0041(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0041[] =
		"\\A.*Descriptor reserve is smaller than logical payload during resize.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0041,
		capture_libmem_inconsistent_resize_string_reserve));

	RETURN_STATUS;
}
