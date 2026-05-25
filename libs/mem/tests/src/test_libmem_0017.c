#include "test_libmem_utils.h"

/**
 * @brief Check string resizing with ZERO_NEW_MEMORY, shrink, and zero-release
 *
 * @return Return describing success or failure
 */
Return test_libmem_0017(void)
{
	INITTEST;

	m_create(char,string_buffer);

	static const char expected[] = "alpha-beta-gamma+delta-epsilon";
	const size_t previous_length = sizeof(expected);

	ASSERT(SUCCESS == m_to_string(string_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(expected),expected));
	ASSERT(SUCCESS == m_resize(string_buffer,previous_length + 8,ZERO_NEW_MEMORY));

	const char *expanded_view = m_data_ro(char,string_buffer);
	ASSERT(expanded_view != NULL);

	IF(expanded_view != NULL)
	{
		bool zero_tail = true;

		for(size_t index = previous_length; index < string_buffer->length; ++index)
		{
			if(expanded_view[index] != '\0')
			{
				zero_tail = false;
				break;
			}
		}

		ASSERT(zero_tail == true);
	}

	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(string_buffer->is_string == true);
	ASSERT(SUCCESS == m_resize(string_buffer,6,RELEASE_UNUSED));
	ASSERT(string_buffer->length == 6);
	ASSERT(string_buffer->string_length == 5);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"alpha"));

	void *const retained_data = string_buffer->data;
	const size_t retained_bytes = string_buffer->actually_allocated_bytes;

	ASSERT(SUCCESS == m_resize(string_buffer,0));
	ASSERT(string_buffer->data == retained_data);
	ASSERT(string_buffer->actually_allocated_bytes == retained_bytes);
	ASSERT(string_buffer->length == 0);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->is_string == true);

	ASSERT(SUCCESS == m_resize(string_buffer,0,RELEASE_UNUSED));
	ASSERT(string_buffer->data == NULL);
	ASSERT(string_buffer->actually_allocated_bytes == 0);
	ASSERT(string_buffer->length == 0);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->is_string == true);
	call(m_del(string_buffer));

	RETURN_STATUS;
}
