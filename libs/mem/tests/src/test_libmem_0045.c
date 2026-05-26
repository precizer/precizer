#include "test_libmem_utils.h"

/**
 * @brief Check the overloaded m_concat_string macro for bounded and unbounded calls
 *
 * @return Return describing success or failure
 */
Return test_libmem_0045(void)
{
	INITTEST;

	static const char base_text[] = "base";
	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));
	m_create(uint32_t,code_units);

	const char unbounded_suffix[] = "-u";
	const char bounded_suffix[] = {'-','b','\0','x'};
	const uint32_t initial_code_units[] = {
		UINT32_C(10),
		UINT32_C(0)
	};
	const uint32_t bounded_wide_suffix[] = {
		UINT32_C(20),
		UINT32_C(0),
		UINT32_C(777)
	};

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == m_concat_string(string_buffer,unbounded_suffix));
	ASSERT(SUCCESS == m_concat_string(string_buffer,sizeof(bounded_suffix),bounded_suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base-u-b"));
	ASSERT(string_buffer->string_length == strlen("base-u-b"));
	ASSERT(string_buffer->is_string == true);

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(SUCCESS == m_concat_string(
		code_units,
		sizeof(bounded_wide_suffix),
		bounded_wide_suffix));
	ASSERT(code_units->length == 3);
	ASSERT(code_units->string_length == 2);
	ASSERT(code_units->is_string == true);

	const uint32_t *code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(code_unit_view != NULL);

	IF(code_unit_view != NULL)
	{
		ASSERT(code_unit_view[0] == UINT32_C(10));
		ASSERT(code_unit_view[1] == UINT32_C(20));
		ASSERT(code_unit_view[2] == UINT32_C(0));
	}

	call(m_del(code_units));
	call(m_del(string_buffer));

	RETURN_STATUS;
}
