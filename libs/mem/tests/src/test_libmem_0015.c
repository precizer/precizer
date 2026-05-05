#include "test_libmem_utils.h"

/**
 * @brief Check string resize and raw writable access keep cached metadata coherent
 *
 * @return Return describing success or failure
 */
Return test_libmem_0015(void)
{
	INITTEST;

	m_create(char,buffer);
	static const char refill[] = "hi";

	ASSERT(SUCCESS == m_resize(buffer,3));

	char *bytes = m_data(char,buffer);
	ASSERT(bytes != NULL);

	if(bytes != NULL)
	{
		bytes[0] = 'x';
		bytes[1] = 'y';
		bytes[2] = 'z';
	}

	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == false);
	ASSERT(SUCCESS == m_to_string(buffer));
	ASSERT(buffer->string_length == 3);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 4);
	ASSERT(0 == strcmp(m_text(buffer),"xyz"));

	ASSERT(SUCCESS == m_resize(buffer,3));
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(buffer),"xy"));

	void *const retained_string_data = buffer->data;
	const size_t retained_string_bytes = buffer->actually_allocated_bytes;

	ASSERT(SUCCESS == m_resize(buffer,0));
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->actually_allocated_bytes == retained_string_bytes);
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(buffer),""));

	ASSERT(SUCCESS == mem_concat_unbounded_string(buffer,refill));
	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 3);
	ASSERT(0 == strcmp(m_text(buffer),"hi"));

	char *raw_bytes = (char *)m_raw_data(buffer);
	ASSERT(raw_bytes != NULL);

	if(raw_bytes != NULL)
	{
		raw_bytes[0] = 'b';
		raw_bytes[1] = 'y';
		raw_bytes[2] = '\0';
	}

	ASSERT(buffer->data == retained_string_data);
	ASSERT(buffer->string_length == 2);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->length == 3);
	ASSERT(0 == strcmp(m_text(buffer),"by"));

	ASSERT(SUCCESS == m_del(buffer));
	ASSERT(buffer->length == 0);
	ASSERT(buffer->string_length == 0);
	ASSERT(buffer->is_string == true);
	ASSERT(buffer->data == NULL);
	ASSERT(buffer->actually_allocated_bytes == 0);

	RETURN_STATUS;
}
