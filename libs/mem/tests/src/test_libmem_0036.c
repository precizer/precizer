#include "test_libmem_all.h"

/**
 * @brief Check byte-string truncation without changing descriptor length
 *
 * @return Return describing success or failure
 */
Return test_libmem_0036(void)
{
	INITTEST;

	static const char alphabet_text[] = "alphabet";
	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(alphabet_text),alphabet_text));

	void *const original_data = string_buffer->data;
	const size_t original_length = string_buffer->length;

	ASSERT(SUCCESS == m_string_truncate(string_buffer,5));
	ASSERT(string_buffer->data == original_data);
	ASSERT(string_buffer->length == original_length);
	ASSERT(string_buffer->string_length == 5);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"alpha"));

	char *mutable_view = (char *)string_buffer->data;
	ASSERT(mutable_view != NULL);

	IF(mutable_view != NULL)
	{
		mutable_view[5] = 'x';
	}

	ASSERT(SUCCESS == m_string_truncate(string_buffer,5));
	ASSERT(string_buffer->string_length == 5);
	ASSERT(0 == strcmp(m_text(string_buffer),"alpha"));

	ASSERT(SUCCESS == m_string_truncate(string_buffer,99));
	ASSERT(string_buffer->string_length == 5);
	ASSERT(0 == strcmp(m_text(string_buffer),"alpha"));

	ASSERT(SUCCESS == m_string_truncate(string_buffer,0));
	ASSERT(string_buffer->data == original_data);
	ASSERT(string_buffer->length == original_length);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),""));

	call(m_del(string_buffer));

	RETURN_STATUS;
}
