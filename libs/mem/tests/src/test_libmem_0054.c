#include "test_libmem_utils.h"

/**
 * @brief Check guarded arithmetic helpers used by libmem internals
 *
 * The two negative cases mark their expected guard failures before
 * provoking them, so the final telemetry summary can distinguish
 * intentional coverage from unexpected arithmetic trouble
 *
 * @return Return describing success or failure
 */
Return test_libmem_0054(void)
{
	INITTEST;

	m_create(char,byte_buffer);
	size_t result = 0;

	ASSERT(SUCCESS == m_guarded_byte_size(byte_buffer,16u,&result));
	ASSERT(result == 16u);

	ASSERT(SUCCESS == m_guarded_add(10u,20u,&result));
	ASSERT(result == 30u);

	ASSERT(SUCCESS == m_guarded_subtract(20u,10u,&result));
	ASSERT(result == 10u);

	telemetry_expected_arithmetic_guard_failures();
	ASSERT(FAILURE == m_guarded_add(SIZE_MAX,1u,&result));

	telemetry_expected_arithmetic_guard_failures();
	ASSERT(FAILURE == m_guarded_subtract(0u,1u,&result));
	call(m_del(byte_buffer));

	RETURN_STATUS;
}
