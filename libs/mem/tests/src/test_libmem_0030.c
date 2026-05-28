#include "test_libmem_all.h"

/**
 * @brief Check public wrappers and fixed-string mode on byte-sized strings
 *
 * @return Return describing success or failure
 */
Return test_libmem_0030(void)
{
	INITTEST;

	static const char base_text[] = "base";
	m_create(char,string_buffer);

	static const char empty_suffix[] = "";
	const char bounded_suffix[] = {'-','b','\0','x'};
	static const char unbounded_suffix[] = "-u";
	const char literal_suffix[] = {'-','l','i','t','\0'};
	static const char expected[] = "base-b-u-lit";

	ASSERT(SUCCESS == m_to_string(string_buffer));
	ASSERT(SUCCESS == m_copy_fixed_string(string_buffer,sizeof(base_text),base_text));
	ASSERT(SUCCESS == m_concat_string(
		string_buffer,
		sizeof(empty_suffix),
		empty_suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(string_buffer->string_length == strlen("base"));
	ASSERT(string_buffer->is_string == true);
	ASSERT(SUCCESS == m_concat_string(string_buffer,0,bounded_suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(SUCCESS == mem_core_string(
		SOURCE_FIXED_STRING | TRANSFER_APPEND,
		string_buffer,
		0,
		literal_suffix));
	ASSERT(0 == strcmp(m_text(string_buffer),"base"));
	ASSERT(SUCCESS == mem_core_string(
		SOURCE_BOUNDED_STRING | TRANSFER_APPEND,
		string_buffer,
		sizeof(bounded_suffix),
		bounded_suffix));
	ASSERT(SUCCESS == m_concat_string(
		string_buffer,
		unbounded_suffix));
	ASSERT(SUCCESS == mem_core_string(
		SOURCE_FIXED_STRING | TRANSFER_APPEND,
		string_buffer,
		sizeof(literal_suffix),
		literal_suffix));
	ASSERT(string_buffer->is_string == true);
	ASSERT(string_buffer->string_length == sizeof(expected) - 1);
	ASSERT(0 == strcmp(m_text(string_buffer),expected));

	call(m_del(string_buffer));

	RETURN_STATUS;
}
