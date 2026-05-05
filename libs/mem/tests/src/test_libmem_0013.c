#include "test_libmem_utils.h"

/**
 * @brief Check create supports optional initial mode selection
 *
 * @return Return describing success or failure
 */
Return test_libmem_0013(void)
{
	INITTEST;

	m_create(char,data_buffer);
	m_create(char,string_buffer,MEMORY_STRING);
	m_create(uint32_t,wide_string,MEMORY_STRING);

	const char literal_suffix[] = "abc";
	const uint32_t wide_literal_suffix[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(0)
	};

	ASSERT(data_buffer->is_string == false);
	ASSERT(data_buffer->string_length == 0);
	ASSERT(data_buffer->length == 0);

	ASSERT(string_buffer->is_string == true);
	ASSERT(string_buffer->string_length == 0);
	ASSERT(string_buffer->length == 0);
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(literal_suffix),literal_suffix));
	ASSERT(string_buffer->string_length == 3);

	const char *string_view = m_data_ro(char,string_buffer);
	ASSERT(string_view != NULL);

	if(string_view != NULL)
	{
		ASSERT(string_view[0] == 'a');
		ASSERT(string_view[1] == 'b');
		ASSERT(string_view[2] == 'c');
		ASSERT(string_view[3] == '\0');
	}

	ASSERT(wide_string->is_string == true);
	ASSERT(wide_string->string_length == 0);
	ASSERT(wide_string->length == 0);
	ASSERT(SUCCESS == m_concat_fixed_string(wide_string,sizeof(wide_literal_suffix),wide_literal_suffix));
	ASSERT(wide_string->string_length == 2);
	ASSERT(wide_string->length == 3);

	const uint32_t *wide_view = m_data_ro(uint32_t,wide_string);
	ASSERT(wide_view != NULL);

	if(wide_view != NULL)
	{
		ASSERT(wide_view[0] == UINT32_C(10));
		ASSERT(wide_view[1] == UINT32_C(20));
		ASSERT(wide_view[2] == UINT32_C(0));
	}

	ASSERT(SUCCESS == m_del(wide_string));
	ASSERT(SUCCESS == m_del(string_buffer));
	ASSERT(SUCCESS == m_del(data_buffer));

	RETURN_STATUS;
}
