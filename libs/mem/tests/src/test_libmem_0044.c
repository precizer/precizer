#include "test_libmem_utils.h"

/**
 * @brief Check fixed-string append and literal append sugar on byte and multi-byte string descriptors
 *
 * @return Return describing success or failure
 */
	static const char base_text[] = "base";
Return test_libmem_0044(void)
{
	INITTEST;

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));
	m_create(uint32_t,code_units);

	const char suffix[] = {'-','l','i','t','\0'};
	static const char empty_suffix[] = "";
	const uint32_t initial_code_units[] = {
		UINT32_C(10),
		UINT32_C(0)
	};
	const uint32_t literal_suffix[] = {
		UINT32_C(20),
		UINT32_C(30),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(suffix),suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base-lit"));
	ASSERT(SUCCESS == m_concat_literal(string_buffer,"!"));
	ASSERT(0 == strcmp(m_text(string_buffer),"base-lit!"));
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(empty_suffix),empty_suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base-lit!"));
	ASSERT(string_buffer->string_length == strlen("base-lit!"));
	ASSERT(string_buffer->is_string == true);

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(SUCCESS == mem_concat_fixed_string(
		code_units,
		sizeof(literal_suffix),
		literal_suffix));
	ASSERT(code_units->length == 4);
	ASSERT(code_units->string_length == 3);
	ASSERT(code_units->is_string == true);

	const uint32_t *code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(code_unit_view != NULL);

	if(code_unit_view != NULL)
	{
		ASSERT(code_unit_view[0] == UINT32_C(10));
		ASSERT(code_unit_view[1] == UINT32_C(20));
		ASSERT(code_unit_view[2] == UINT32_C(30));
		ASSERT(code_unit_view[3] == UINT32_C(0));
	}

	ASSERT(SUCCESS == m_del(code_units));
	ASSERT(SUCCESS == m_del(string_buffer));

	RETURN_STATUS;
}
