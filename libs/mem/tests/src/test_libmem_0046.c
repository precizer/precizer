#include "test_libmem_utils.h"

/**
 * @brief Check the overloaded m_copy_string macro for unbounded and bounded replacement
 *
 * @return Return describing success or failure
 */
	static const char abcdef_text[] = "abcdef";
Return test_libmem_0046(void)
{
	INITTEST;

	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));
	m_create(uint32_t,wide_buffer);
	ASSERT(SUCCESS == m_to_string(wide_buffer));

	const char unbounded_source[] = "alpha";
	const char bounded_source[] = {'b','e','t','a','\0','x'};
	const uint32_t wide_bounded_source[] = {
		UINT32_C(10),
		UINT32_C(20),
		UINT32_C(0),
		UINT32_C(777)
	};

	ASSERT(SUCCESS == m_copy_string(string_buffer,unbounded_source));
	ASSERT(0 == strcmp(m_text(string_buffer),"alpha"));
	ASSERT(string_buffer->string_length == strlen("alpha"));
	ASSERT(string_buffer->is_string == true);

	ASSERT(SUCCESS == m_copy_string(string_buffer,sizeof(bounded_source),bounded_source));
	ASSERT(0 == strcmp(m_text(string_buffer),"beta"));
	ASSERT(string_buffer->string_length == strlen("beta"));
	ASSERT(string_buffer->is_string == true);

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(abcdef_text),abcdef_text));

	const char *aliased_source = m_text(string_buffer);
	ASSERT(aliased_source != NULL);

	if(aliased_source != NULL)
	{
		aliased_source += 2;
		ASSERT(SUCCESS == m_copy_string(string_buffer,aliased_source));
	}

	ASSERT(string_buffer->length == 5);
	ASSERT(string_buffer->string_length == 4);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"cdef"));

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(abcdef_text),abcdef_text));

	aliased_source = m_text(string_buffer);
	ASSERT(aliased_source != NULL);

	if(aliased_source != NULL)
	{
		aliased_source += 2;
		ASSERT(SUCCESS == m_copy_string(
			string_buffer,
			string_buffer->length - 2,
			aliased_source));
	}

	ASSERT(string_buffer->length == 5);
	ASSERT(string_buffer->string_length == 4);
	ASSERT(string_buffer->is_string == true);
	ASSERT(0 == strcmp(m_text(string_buffer),"cdef"));

	ASSERT(SUCCESS == m_copy_string(
		wide_buffer,
		sizeof(wide_bounded_source),
		wide_bounded_source));
	ASSERT(wide_buffer->length == 3);
	ASSERT(wide_buffer->string_length == 2);
	ASSERT(wide_buffer->is_string == true);

	const uint32_t *wide_view = m_data_ro(uint32_t,wide_buffer);
	ASSERT(wide_view != NULL);

	if(wide_view != NULL)
	{
		ASSERT(wide_view[0] == UINT32_C(10));
		ASSERT(wide_view[1] == UINT32_C(20));
		ASSERT(wide_view[2] == UINT32_C(0));
	}

	ASSERT(SUCCESS == m_del(wide_buffer));
	ASSERT(SUCCESS == m_del(string_buffer));

	RETURN_STATUS;
}
