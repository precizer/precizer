#include "test_libmem_utils.h"

/**
 * @brief Check data-to-string conversion adds a terminator for multi-byte payloads without one
 *
 * @return Return describing success or failure
 */
Return test_libmem_0025(void)
{
	INITTEST;

	m_create(uint32_t,code_units);

	ASSERT(SUCCESS == m_resize(code_units,3));

	uint32_t *code_unit_view = m_data(uint32_t,code_units);
	ASSERT(code_unit_view != NULL);

	IF(code_unit_view != NULL)
	{
		code_unit_view[0] = UINT32_C(0x00010000);
		code_unit_view[1] = UINT32_C(0x00000001);
		code_unit_view[2] = UINT32_C(0x00000002);
	}

	ASSERT(SUCCESS == m_to_string(code_units));
	ASSERT(code_units->length == 4);
	ASSERT(code_units->string_length == 3);
	ASSERT(code_units->is_string == true);

	const uint32_t *const_code_unit_view = m_data_ro(uint32_t,code_units);
	ASSERT(const_code_unit_view != NULL);

	IF(const_code_unit_view != NULL)
	{
		ASSERT(const_code_unit_view[0] == UINT32_C(0x00010000));
		ASSERT(const_code_unit_view[1] == UINT32_C(0x00000001));
		ASSERT(const_code_unit_view[2] == UINT32_C(0x00000002));
		ASSERT(const_code_unit_view[3] == UINT32_C(0x00000000));
	}

	call(m_del(code_units));

	RETURN_STATUS;
}
