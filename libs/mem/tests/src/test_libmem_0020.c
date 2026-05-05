#include "test_libmem_utils.h"

/**
 * @brief Check explicit data/string mode conversion helpers
 *
 * @return Return describing success or failure
 */
	static const char xyz_text[] = "xyz";
Return test_libmem_0020(void)
{
	INITTEST;

	m_create(char,buffer);

	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(buffer,sizeof(xyz_text),xyz_text));
	ASSERT(buffer->length == 4);
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->is_string == true);

	ASSERT(SUCCESS == m_to_data(buffer));
	ASSERT(buffer->length == 3);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);

	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(buffer->length == 4);
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(buffer),"xyz"));

	ASSERT(SUCCESS == m_del(buffer));

	RETURN_STATUS;
}
