#include "test_libmem_utils.h"

/**
 * @brief Check internal replace mode on multi-byte strings and aliased empty replacement
 *
 * @return Return describing success or failure
 */
Return test_libmem_0043(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	const uint32_t initial_code_units[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(30),
		UINT32_C(40),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));

	const uint32_t *aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += 2;
		ASSERT(SUCCESS == mem_core_string(
				SOURCE_BOUNDED_STRING | TRANSFER_REPLACE,
			code_units,
			code_units->length * sizeof(uint32_t),
				aliased_suffix));
	}

	ASSERT(code_units->length == 3);
	ASSERT(code_units->string_length == 2);
	ASSERT(code_units->is_string == true);

	const uint32_t *const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(30));
		ASSERT(const_code_unit_view[1] == UINT32_C(40));
		ASSERT(const_code_unit_view[2] == UINT32_C(0));
	}

	aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	size_t code_units_length = 0U;
	ASSERT(SUCCESS == m_string_length(code_units,&code_units_length));

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += code_units_length;
		ASSERT(SUCCESS == mem_core_string(
				SOURCE_UNBOUNDED_STRING | TRANSFER_REPLACE,
			code_units,
			0,
				aliased_suffix));
	}

	ASSERT(code_units->length == 1);
	ASSERT(code_units->string_length == 0);
	ASSERT(code_units->is_string == true);

	const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(0));
	}

	call(m_del(code_units));

	RETURN_STATUS;
}
