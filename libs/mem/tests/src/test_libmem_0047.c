#include "test_libmem_utils.h"

/**
 * @brief Check fixed-string replacement wrappers and literal replacement sugar for byte and wide descriptors
 *
 * @return Return describing success or failure
 */
	static const char abcdef_text[] = "abcdef";
Return test_libmem_0047(void)
{
	INITTEST;

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));
	m_create(uint32_t,wide_buffer);
	ASSERT(SUCCESS == m_to_string(wide_buffer));

	const char title_text[] = "draft";
	const uint32_t wide_literal[] = {
		UINT32_C(100),
		UINT32_C(200),
		UINT32_C(0)
	};

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(title_text),title_text));
	ASSERT(string_buffer->length == sizeof(title_text));
	ASSERT(string_buffer->string_length == strlen("draft"));
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"draft"));
	ASSERT(SUCCESS == m_copy_literal(string_buffer,"title"));
	ASSERT(string_buffer->length == sizeof("title"));
	ASSERT(string_buffer->string_length == strlen("title"));
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"title"));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(abcdef_text),abcdef_text));

	const char *aliased_literal = m_text(string_buffer);
	ASSERT(aliased_literal != NULL);

	IF(aliased_literal != NULL)
	{
		aliased_literal += 2;
		ASSERT(SUCCESS == m_copy_fixed_string(
			string_buffer,
			string_buffer->length - 2,
			aliased_literal));
	}

	ASSERT(string_buffer->length == 5);
	ASSERT(string_buffer->string_length == 4);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"cdef"));

	ASSERT(SUCCESS == m_copy_fixed_string(wide_buffer,sizeof(wide_literal),wide_literal));
	ASSERT(wide_buffer->length == 3);
	ASSERT(wide_buffer->string_length == 2);
	ASSERT(wide_buffer->is_string == true);

	const uint32_t *wide_view = m_data_ro(uint32_t,wide_buffer);
	ASSERT(wide_view != NULL);

	IF(wide_view != NULL)
	{
		ASSERT(wide_view[0] == UINT32_C(100));
		ASSERT(wide_view[1] == UINT32_C(200));
		ASSERT(wide_view[2] == UINT32_C(0));
	}

	call(m_del(wide_buffer));
	call(m_del(string_buffer));

	RETURN_STATUS;
}
