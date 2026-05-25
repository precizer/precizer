#include "test_libmem_utils.h"


/**
 * @brief Capture string truncate rejection for descriptors whose reserve does not cover the payload
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_inconsistent_string_truncate_reserve(void)
{
	INITTEST;

	char materialized_same_length[] = "alpha";
	char materialized_growth_noop[] = "omega";
	memory invalid_same_length = m_init(char,MEMORY_STRING);
	memory invalid_growth_noop = m_init(char,MEMORY_STRING);

	invalid_same_length.data = materialized_same_length;
	invalid_same_length.length = sizeof(materialized_same_length);
	invalid_same_length.actually_allocated_bytes = sizeof(materialized_same_length) - 1u;
	invalid_same_length.string_length = sizeof(materialized_same_length) - 1u;
	invalid_same_length.is_string = true;

	invalid_growth_noop.data = materialized_growth_noop;
	invalid_growth_noop.length = sizeof(materialized_growth_noop);
	invalid_growth_noop.actually_allocated_bytes = sizeof(materialized_growth_noop) - 1u;
	invalid_growth_noop.string_length = sizeof(materialized_growth_noop) - 1u;
	invalid_growth_noop.is_string = true;

	ASSERT(FAILURE == mem_string_truncate(&invalid_same_length,invalid_same_length.string_length));
	ASSERT(invalid_same_length.data == materialized_same_length);
	ASSERT(invalid_same_length.length == sizeof(materialized_same_length));
	ASSERT(invalid_same_length.actually_allocated_bytes == sizeof(materialized_same_length) - 1u);
	ASSERT(invalid_same_length.string_length == sizeof(materialized_same_length) - 1u);
	ASSERT(invalid_same_length.is_string == true);

	ASSERT(FAILURE == mem_string_truncate(&invalid_growth_noop,invalid_growth_noop.string_length + 1u));
	ASSERT(invalid_growth_noop.data == materialized_growth_noop);
	ASSERT(invalid_growth_noop.length == sizeof(materialized_growth_noop));
	ASSERT(invalid_growth_noop.actually_allocated_bytes == sizeof(materialized_growth_noop) - 1u);
	ASSERT(invalid_growth_noop.string_length == sizeof(materialized_growth_noop) - 1u);
	ASSERT(invalid_growth_noop.is_string == true);

	deliver(status);
}

/**
 * @brief Check string truncate rejection for descriptors whose reserve does not cover the payload
 *
 * @return Return describing success or failure
 */
Return test_libmem_0042(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0042[] =
		"\\A.*Descriptor reserve is smaller than logical payload during string truncate.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0042,
		capture_libmem_inconsistent_string_truncate_reserve));

	RETURN_STATUS;
}
