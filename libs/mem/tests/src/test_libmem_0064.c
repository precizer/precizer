#include "test_libmem_all.h"

/**
 * @brief Check bounded raw and bounded string concat helpers directly
 *
 * @return Return describing success or failure
 */
Return test_libmem_0064(void)
{
	INITTEST;

	static const char pre_text[] = "pre";
	static const char go_text[] = "go";

	m_create(char,raw_destination);
	m_create(char,string_destination);
	ASSERT(SUCCESS == m_to_string(string_destination));
	m_create(char,raw_wrapper_destination);
	m_create(char,string_wrapper_destination);
	ASSERT(SUCCESS == m_to_string(string_wrapper_destination));

	const char raw_prefix[] = {'a','b','\0'};
	const char raw_suffix[] = {'c','\0','d'};
	const char bounded_string_with_terminator[] = {'-','x','\0','z'};
	const char bounded_string_without_terminator[] = {'-','o','k'};
	const char wrapper_raw_suffix[] = {'1','\0','2'};
	const char wrapper_string_suffix[] = {'!','?','\0','x'};

	ASSERT(SUCCESS == m_copy_buffer(raw_destination,sizeof(raw_prefix),raw_prefix));
	ASSERT(SUCCESS == m_concat_buffer(raw_destination,sizeof(raw_suffix),raw_suffix));
	ASSERT(raw_destination->string_length == 0);
	ASSERT(raw_destination->is_string == false);
	ASSERT(raw_destination->length == sizeof(raw_prefix) + sizeof(raw_suffix));

	const unsigned char *aliased_raw_suffix = (const unsigned char *)m_raw_data_ro(raw_destination) + 1;
	ASSERT(SUCCESS == m_concat_buffer(raw_destination,4,aliased_raw_suffix));
	ASSERT(raw_destination->string_length == 0);
	ASSERT(raw_destination->is_string == false);
	ASSERT(raw_destination->length == sizeof(raw_prefix) + sizeof(raw_suffix) + 4);

	const unsigned char *raw_view = (const unsigned char *)m_raw_data_ro(raw_destination);
	ASSERT(raw_view != NULL);

	IF(raw_view != NULL)
	{
		ASSERT(raw_view[0] == (unsigned char)'a');
		ASSERT(raw_view[1] == (unsigned char)'b');
		ASSERT(raw_view[2] == (unsigned char)'\0');
		ASSERT(raw_view[3] == (unsigned char)'c');
		ASSERT(raw_view[4] == (unsigned char)'\0');
		ASSERT(raw_view[5] == (unsigned char)'d');
		ASSERT(raw_view[6] == (unsigned char)'b');
		ASSERT(raw_view[7] == (unsigned char)'\0');
		ASSERT(raw_view[8] == (unsigned char)'c');
		ASSERT(raw_view[9] == (unsigned char)'\0');
	}

	ASSERT(SUCCESS == m_copy_fixed_string(string_destination,sizeof(pre_text),pre_text));
	ASSERT(SUCCESS == m_concat_string(
		string_destination,
		sizeof(bounded_string_with_terminator),
		bounded_string_with_terminator));
	ASSERT(string_destination->string_length == 5);
	ASSERT(string_destination->is_string == true);
	ASSERT(0 == strcmp(m_text(string_destination),"pre-x"));
	ASSERT(SUCCESS == m_concat_string(
		string_destination,
		sizeof(bounded_string_without_terminator),
		bounded_string_without_terminator));
	ASSERT(string_destination->string_length == 8);
	ASSERT(string_destination->is_string == true);
	ASSERT(0 == strcmp(m_text(string_destination),"pre-x-ok"));

	const char *aliased_string_suffix = m_text(string_destination) + 4;
	ASSERT(SUCCESS == m_concat_string(
		string_destination,
		string_destination->length - 4,
		aliased_string_suffix));
	ASSERT(string_destination->string_length == 12);
	ASSERT(string_destination->is_string == true);
	ASSERT(0 == strcmp(m_text(string_destination),"pre-x-okx-ok"));

	ASSERT(SUCCESS == m_copy_buffer(raw_wrapper_destination,sizeof(raw_prefix),raw_prefix));
	ASSERT(SUCCESS == m_concat_buffer(raw_wrapper_destination,sizeof(wrapper_raw_suffix),wrapper_raw_suffix));
	ASSERT(raw_wrapper_destination->string_length == 0);
	ASSERT(raw_wrapper_destination->is_string == false);
	ASSERT(raw_wrapper_destination->length == sizeof(raw_prefix) + sizeof(wrapper_raw_suffix));

	const unsigned char *raw_wrapper_view = (const unsigned char *)m_raw_data_ro(raw_wrapper_destination);
	ASSERT(raw_wrapper_view != NULL);

	IF(raw_wrapper_view != NULL)
	{
		ASSERT(raw_wrapper_view[0] == (unsigned char)'a');
		ASSERT(raw_wrapper_view[1] == (unsigned char)'b');
		ASSERT(raw_wrapper_view[2] == (unsigned char)'\0');
		ASSERT(raw_wrapper_view[3] == (unsigned char)'1');
		ASSERT(raw_wrapper_view[4] == (unsigned char)'\0');
		ASSERT(raw_wrapper_view[5] == (unsigned char)'2');
	}

	ASSERT(SUCCESS == m_copy_fixed_string(string_wrapper_destination,sizeof(go_text),go_text));
	ASSERT(SUCCESS == m_concat_string(
		string_wrapper_destination,
		sizeof(wrapper_string_suffix),
		wrapper_string_suffix));
	ASSERT(string_wrapper_destination->string_length == 4);
	ASSERT(string_wrapper_destination->is_string == true);
	ASSERT(0 == strcmp(m_text(string_wrapper_destination),"go!?"));

	call(m_del(raw_destination));
	call(m_del(string_destination));
	call(m_del(raw_wrapper_destination));
	call(m_del(string_wrapper_destination));

	RETURN_STATUS;
}
