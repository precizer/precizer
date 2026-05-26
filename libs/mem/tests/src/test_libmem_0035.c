#include "test_libmem_utils.h"

/**
 * @brief Capture the internal bounded-source negative case past the visible terminator
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_invalid_internal_bounded_source(void)
{
	INITTEST;

	static const char base_text[] = "base";

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == m_resize(string_buffer,string_buffer->length + 2,ZERO_NEW_MEMORY));

	size_t string_buffer_length = 0U;
	ASSERT(SUCCESS == m_string_length(string_buffer,&string_buffer_length));

	const char *invalid_internal_source = m_text(string_buffer) + string_buffer_length + 1;

	ASSERT(invalid_internal_source != NULL);

	IF(invalid_internal_source != NULL)
	{
		ASSERT(FAILURE == m_concat_string(
			string_buffer,
			1,
			invalid_internal_source));
	}

	call(m_del(string_buffer));

	deliver(status);
}

/**
 * @brief Check internal bounded mode still rejects starts past the visible terminator
 *
 * @return Return describing success or failure
 */
Return test_libmem_0035(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0035[] =
	        "\\A.*Source start exceeds destination visible string bounds.*\\Z";

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0035,
		capture_libmem_invalid_internal_bounded_source));

	RETURN_STATUS;
}
