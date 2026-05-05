#include "test_libmem_utils.h"

/**
 * @brief Check the string copy and concat helpers on one descriptor
 *
 * @return Return describing success or failure
 */
	static const char alpha_text[] = "alpha";
	static const char plus_delta_text[] = "+delta";
Return test_libmem_0016(void)
{
	INITTEST;

	m_create(char,string_buffer);
	m_create(char,extra_buffer);

	const char beta_suffix[] = "-beta";
	const char bounded_gamma[] = {'-','g','a','m','m','a','\0','x'};
	const char bounded_suffix[] = {'-','e','p','s','i','l','o','n','\0','x','x'};
	static const char expected[] = "alpha-beta-gamma+delta-epsilon";

	ASSERT(SUCCESS == m_to_string(string_buffer));
	ASSERT(SUCCESS == m_to_string(extra_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(alpha_text),alpha_text));
	ASSERT(SUCCESS == m_concat_fixed_string(string_buffer,sizeof(beta_suffix),beta_suffix));
	ASSERT(SUCCESS == m_copy_string(extra_buffer,sizeof(bounded_gamma),bounded_gamma));
	ASSERT(SUCCESS == m_concat_strings(string_buffer,extra_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(extra_buffer,sizeof(plus_delta_text),plus_delta_text));
	ASSERT(SUCCESS == m_concat_strings(string_buffer,extra_buffer));
	ASSERT(SUCCESS == m_concat_string(string_buffer,sizeof(bounded_suffix),bounded_suffix));

	const char *string_view = m_data_ro(char,string_buffer);
	ASSERT(string_view != NULL);

	if(string_view != NULL)
	{
		ASSERT(0 == strcmp(string_view,expected));
	}

	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(string_buffer->is_string == true);
	ASSERT(extra_buffer->string_length == 6);
	ASSERT(extra_buffer->is_string == true);
	ASSERT(SUCCESS == m_del(string_buffer));
	ASSERT(SUCCESS == m_del(extra_buffer));

	RETURN_STATUS;
}
