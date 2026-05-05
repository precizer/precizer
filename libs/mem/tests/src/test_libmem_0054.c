#include "test_libmem_utils.h"

/**
 * @brief Check guarded arithmetic helpers used by libmem internals
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

	ASSERT(FAILURE == m_guarded_add(SIZE_MAX,1u,&result));
	ASSERT(FAILURE == m_guarded_subtract(0u,1u,&result));
	ASSERT(SUCCESS == m_del(byte_buffer));

	RETURN_STATUS;
}
