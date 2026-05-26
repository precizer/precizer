#include "test_libmem_utils.h"

/**
 * @brief Check string resizing with ZERO_NEW_MEMORY, shrink, and zero-release
 *
 * Builds a string descriptor, temporarily exposes extra tail slots, and
 * fills them with non-zero marker bytes while leaving the existing string
 * terminator intact. After hiding that tail inside retained storage, the
 * test grows the descriptor again with ZERO_NEW_MEMORY and verifies that
 * every re-exposed marker byte was cleared. It then verifies visible-string
 * shrink behavior, zero-length resize with retained storage, and final
 * release of that storage through RELEASE_UNUSED
 *
 * @return Return describing success or failure
 */
Return test_libmem_0017(void)
{
	INITTEST;

	m_create(char,string_buffer);

	static const char expected[] = "alpha-beta-gamma+delta-epsilon";
	const size_t previous_length = sizeof(expected);
	const size_t expanded_length = previous_length + 8U;

	ASSERT(SUCCESS == m_to_string(string_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(expected),expected));

	/* Prepare a deliberately dirty hidden tail before testing ZERO_NEW_MEMORY.
	   A newly allocated buffer may already happen to contain zero bytes, so
	   checking it directly would not prove that the flag cleared anything.
	   These marker bytes make a missing zero-fill immediately visible while
	   leaving the real string and its terminator unchanged */
	ASSERT(SUCCESS == m_resize(string_buffer,expanded_length));

	char *tail_writer = m_data(char,string_buffer);
	ASSERT(tail_writer != NULL);

	IF(tail_writer != NULL)
	{
		for(size_t index = previous_length; index < expanded_length; ++index)
		{
			tail_writer[index] = 'X';
		}
	}

	ASSERT(SUCCESS == m_resize(string_buffer,previous_length));
	ASSERT(0 == strcmp(m_text(string_buffer),expected));

	/* Re-expose the hidden marker bytes: ZERO_NEW_MEMORY must turn them into zeros */
	ASSERT(SUCCESS == m_resize(string_buffer,expanded_length,ZERO_NEW_MEMORY));

	const char *expanded_view = m_data_ro(char,string_buffer);
	ASSERT(expanded_view != NULL);

	IF(expanded_view != NULL)
	{
		bool zero_tail = true;

		for(size_t index = previous_length; index < expanded_length; ++index)
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
