#include "test_libmem_all.h"

/**
 * @brief Check data-mode resize keeps string metadata cleared
 *
 * @return Return describing success or failure
 */
Return test_libmem_0056(void)
{
	INITTEST;

	m_create(unsigned char,packet);

	ASSERT(packet->string_length == 0);
	ASSERT(packet->is_string == false);

	ASSERT(SUCCESS == m_resize(packet,8u));
	ASSERT(packet->length == 8u);
	ASSERT(packet->string_length == 0);
	ASSERT(packet->is_string == false);

	ASSERT(SUCCESS == m_resize(packet,16u,ZERO_NEW_MEMORY));
	ASSERT(packet->length == 16u);
	ASSERT(packet->string_length == 0);
	ASSERT(packet->is_string == false);

	ASSERT(SUCCESS == m_resize(packet,4u,RELEASE_UNUSED));
	ASSERT(packet->length == 4u);
	ASSERT(packet->string_length == 0);
	ASSERT(packet->is_string == false);

	call(m_del(packet));

	RETURN_STATUS;
}
