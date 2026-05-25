#include "test_libmem_utils.h"

/**
 * @brief Check unbounded mode ignores the size argument
 *
 * @return Return describing success or failure
 */
	static const char base_text[] = "base";
Return test_libmem_0032(void)
{
	INITTEST;

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	static const char suffix[] = "suffix";

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == mem_core_string(
			SOURCE_UNBOUNDED_STRING | TRANSFER_APPEND,
		string_buffer,
		1,
			suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"basesuffix"));
	ASSERT(string_buffer->string_length == strlen("basesuffix"));
	ASSERT(string_buffer->is_string == true);
	call(m_del(string_buffer));

	RETURN_STATUS;
}
