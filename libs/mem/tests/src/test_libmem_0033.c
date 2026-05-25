#include "test_libmem_utils.h"


/**
 * @brief Capture the internal unbounded-source negative case past the logical end
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_invalid_internal_unbounded_source(void)
{
	INITTEST;

	static const char base_text[] = "base";

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));

	const char *invalid_internal_source = m_text(string_buffer) + string_buffer->length;

	ASSERT(invalid_internal_source != NULL);

	IF(invalid_internal_source != NULL)
	{
		ASSERT(FAILURE == mem_core_string(
				SOURCE_UNBOUNDED_STRING | TRANSFER_APPEND,
			string_buffer,
			0,
				invalid_internal_source));
	}

	call(m_del(string_buffer));

	deliver(status);
}

/**
 * @brief Check internal unbounded mode rejects one-past-end logical pointers
 *
 * @return Return describing success or failure
 */
Return test_libmem_0033(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0033[] =
		"\\A.*Unbounded source start exceeds destination logical bounds.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0033,
		capture_libmem_invalid_internal_unbounded_source));

	RETURN_STATUS;
}
