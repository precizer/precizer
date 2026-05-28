#include "test_libmem_all.h"

/**
 * @brief Check string_length on multi-byte descriptors in both data and string mode
 *
 * @return Return describing success or failure
 */
Return test_libmem_0023(void)
{
	INITTEST;

	m_create(uint32_t,code_units);
	size_t measured_length = 0;

	ASSERT(SUCCESS == m_resize(code_units,4));

	uint32_t *code_unit_view = m_data(uint32_t,code_units);
	ASSERT(code_unit_view != NULL);

	IF(code_unit_view != NULL)
	{
		code_unit_view[0] = UINT32_C(0x00010000);
		code_unit_view[1] = UINT32_C(0x01000000);
		code_unit_view[2] = UINT32_C(0x00000000);
		code_unit_view[3] = UINT32_C(0x00000002);
	}

	ASSERT(SUCCESS == m_string_length(code_units,&measured_length));
	ASSERT(measured_length == 2);

	ASSERT(SUCCESS == m_resize(code_units,3));

	code_unit_view = m_data(uint32_t,code_units);
	ASSERT(code_unit_view != NULL);

	IF(code_unit_view != NULL)
	{
		code_unit_view[0] = UINT32_C(0x00010000);
		code_unit_view[1] = UINT32_C(0x00000001);
		code_unit_view[2] = UINT32_C(0x00000002);
	}

	ASSERT(SUCCESS == m_string_length(code_units,&measured_length));
	ASSERT(measured_length == 3);

	code_units->string_length = 2;
	code_units->is_string = true;

	ASSERT(SUCCESS == m_string_length(code_units,&measured_length));
	ASSERT(measured_length == 2);

	call(m_del(code_units));

	RETURN_STATUS;
}
