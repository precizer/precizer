#include "test_libmem_utils.h"

/**
 * @brief Check NULL sources and zero-sized bounded or fixed-string inputs are no-ops
 *
 * @return Return describing success or failure
 */
Return test_libmem_0034(void)
{
	INITTEST;

	static const char base_text[] = "base";
	m_create(char,string_buffer);
	ASSERT(SUCCESS == m_to_string(string_buffer));

	const char bounded_suffix[] = {'-','b','\0','x'};
	const char literal_suffix[] = {'-','l','i','t','\0'};

	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == mem_concat_unbounded_string(string_buffer,NULL));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(SUCCESS == m_concat_string(string_buffer,sizeof(bounded_suffix),NULL));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(SUCCESS == mem_core_string(
			SOURCE_FIXED_STRING | TRANSFER_APPEND,
		string_buffer,
		sizeof(literal_suffix),
			NULL));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(SUCCESS == mem_core_string(
			SOURCE_UNBOUNDED_STRING | TRANSFER_APPEND,
		string_buffer,
		sizeof(literal_suffix),
			NULL));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	call(m_del(string_buffer));

	RETURN_STATUS;
}
