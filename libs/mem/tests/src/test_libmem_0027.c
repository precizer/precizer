#include "test_libmem_all.h"

/**
 * @brief Check bounded string concat on multi-byte buffers and softly clamped self-aliasing
 *
 * @return Return describing success or failure
 */
Return test_libmem_0027(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	const uint32_t initial_code_units[] = {
		UINT32_C(0x00010000),
		UINT32_C(0x00000001),
		UINT32_C(0x00000000)
	};
	const uint32_t bounded_suffix[] = {
		UINT32_C(0x01000000),
		UINT32_C(0x00000002),
		UINT32_C(0x00000000),
		UINT32_C(0x00000009)
	};

	ASSERT(SUCCESS == m_copy_buffer(code_units,sizeof(initial_code_units),initial_code_units));
	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(SUCCESS == m_concat_string(code_units,sizeof(bounded_suffix),bounded_suffix));
	ASSERT(code_units->length == 5);
	ASSERT(code_units->string_length == 4);
	ASSERT(code_units->is_string == true);

	const uint32_t *const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(0x00010000));
		ASSERT(const_code_unit_view[1] == UINT32_C(0x00000001));
		ASSERT(const_code_unit_view[2] == UINT32_C(0x01000000));
		ASSERT(const_code_unit_view[3] == UINT32_C(0x00000002));
		ASSERT(const_code_unit_view[4] == UINT32_C(0x00000000));
	}

	const uint32_t *aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += 2;
		ASSERT(SUCCESS == m_concat_string(code_units,code_units->length * sizeof(uint32_t),aliased_suffix));
	}

	ASSERT(code_units->length == 7);
	ASSERT(code_units->string_length == 6);
	ASSERT(code_units->is_string == true);

	const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(0x00010000));
		ASSERT(const_code_unit_view[1] == UINT32_C(0x00000001));
		ASSERT(const_code_unit_view[2] == UINT32_C(0x01000000));
		ASSERT(const_code_unit_view[3] == UINT32_C(0x00000002));
		ASSERT(const_code_unit_view[4] == UINT32_C(0x01000000));
		ASSERT(const_code_unit_view[5] == UINT32_C(0x00000002));
		ASSERT(const_code_unit_view[6] == UINT32_C(0x00000000));
	}

	aliased_suffix = m_data_ro(uint32_t,code_units);
	ASSERT(aliased_suffix != NULL);

	size_t code_units_length = 0U;
	ASSERT(SUCCESS == m_string_length(code_units,&code_units_length));

	IF(aliased_suffix != NULL)
	{
		aliased_suffix += code_units_length;
		ASSERT(SUCCESS == m_concat_string(code_units,sizeof(uint32_t),aliased_suffix));
	}

	ASSERT(code_units->length == 7);
	ASSERT(code_units->string_length == 6);
	ASSERT(code_units->is_string == true);

	call(m_del(code_units));

	RETURN_STATUS;
}
