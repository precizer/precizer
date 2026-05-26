#include "test_libmem_utils.h"

/**
 * @brief Capture the m_string_truncate negative case for data-mode descriptors
 *
 * @return Return describing success or failure
 */
static Return capture_libmem_invalid_string_truncate_mode(void)
{
	INITTEST;

	m_create(char,data_buffer);

	ASSERT(SUCCESS == m_copy_buffer(data_buffer,sizeof("raw"),"raw"));
	ASSERT(FAILURE == m_string_truncate(data_buffer,1));
	call(m_del(data_buffer));

	deliver(status);
}

/**
 * @brief Check multi-byte truncation and reject data-mode descriptors
 *
 * @return Return describing success or failure
 */
Return test_libmem_0037(void)
{
	INITTEST;

	static const char expected_stderr_pattern_libmem_0037[] =
	        "\\A.*string truncate requires a string descriptor.*\\Z";

	m_create(uint32_t,code_units);

	const uint32_t source_units[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(30),
		UINT32_C(40),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(source_units),source_units));
	ASSERT(SUCCESS == m_to_string(code_units));

	const size_t original_length = code_units->length;

	ASSERT(SUCCESS == m_string_truncate(code_units,2));
	ASSERT(code_units->length == original_length);
	ASSERT(code_units->string_length == 2);
	ASSERT(code_units->is_string == true);

	const uint32_t *truncated_view = m_data_ro(uint32_t,code_units);
	ASSERT(truncated_view != NULL);

	IF(truncated_view != NULL)
	{
		ASSERT(truncated_view[0] == UINT32_C(10));
		ASSERT(truncated_view[1] == UINT32_C(20));
		ASSERT(truncated_view[2] == UINT32_C(0));
	}

	ASSERT(SUCCESS == m_string_truncate(code_units,8));
	ASSERT(code_units->string_length == 2);

	ASSERT(SUCCESS == match_function_output(
		NULL,
		expected_stderr_pattern_libmem_0037,
		capture_libmem_invalid_string_truncate_mode));

	call(m_del(code_units));

	RETURN_STATUS;
}
