#include "test_libmem_all.h"

/**
 * @brief Check the public unbounded wrapper on multi-byte strings and internal sources
 *
 * @return Return describing success or failure
 */
Return test_libmem_0031(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	const uint32_t initial_code_units[] = {
		UINT32_C(10),
		UINT32_C(0)
	};
	const uint32_t unbounded_suffix[] = {
		UINT32_C(20),
		UINT32_C(30),
		UINT32_C(0)
	};
	const uint32_t literal_suffix[] = {
		UINT32_C(40),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(SUCCESS == m_concat_string(
		code_units,
		unbounded_suffix));
	ASSERT(SUCCESS == mem_core_string(
		SOURCE_FIXED_STRING | TRANSFER_APPEND,
		code_units,
		sizeof(literal_suffix),
		literal_suffix));

	const uint32_t *aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += 2;
		ASSERT(SUCCESS == m_concat_string(
			code_units,
			aliased_suffix));
	}

	aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	size_t code_units_length = 0U;
	ASSERT(SUCCESS == m_string_length(code_units,&code_units_length));

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += code_units_length;
		ASSERT(SUCCESS == m_concat_string(
			code_units,
			aliased_suffix));
	}

	ASSERT(code_units->length == 7);
	ASSERT(code_units->string_length == 6);
	ASSERT(code_units->is_string == true);

	const uint32_t *const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(10));
		ASSERT(const_code_unit_view[1] == UINT32_C(20));
		ASSERT(const_code_unit_view[2] == UINT32_C(30));
		ASSERT(const_code_unit_view[3] == UINT32_C(40));
		ASSERT(const_code_unit_view[4] == UINT32_C(30));
		ASSERT(const_code_unit_view[5] == UINT32_C(40));
		ASSERT(const_code_unit_view[6] == UINT32_C(0));
	}

	call(m_del(code_units));

	RETURN_STATUS;
}
